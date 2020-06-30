#include <netdb.h>
#include <netinet/in.h>
#include <pipeline/pipeline.h>
#include <pipeline/worker.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

#define BUFSIZE (4096 << 4)

#include "ryoan_client_common.inc"

const char *prog_name = "email_pipeline_client";

int64_t iters = 1;
channel_t *chan = NULL;

struct thread_info {
  bool enc;
  bool io_model;
};

void read_file(int fd, char *buf, int64_t len, const char *fname) {
  char *curr = buf;
  while (len > 0) {
    int64_t s = read(fd, curr, len);
    if (s <= 0) {
      fprintf(stderr, "ERROR reading %s\n", fname);
      exit(1);
    }
    len -= s;
    curr += s;
  }
}

void alloc_req_buf(const char *textfile, const char *attachmentfile) {
  int64_t text_len = 0, attachment_len = 0;
  int tfd, afd = -1;
  struct stat st;

  int64_t req_len = sizeof(email_request_info);
  tfd = open(textfile, O_RDONLY);
  if (tfd < 0) {
    fprintf(stderr, "ERROR, cannot open file %s\n", textfile);
    exit(1);
  }
  if (fstat(tfd, &st)) {
    fprintf(stderr, "ERROR, cannot stat file %s\n", textfile);
    exit(1);
  }
  text_len = st.st_size;
  req_len += text_len + 1;  // append '\0' at end of text

  if (attachmentfile) {
    afd = open(attachmentfile, O_RDONLY);
    if (afd < 0) {
      fprintf(stderr, "ERROR, cannot open file %s\n", attachmentfile);
      exit(1);
    }
    if (fstat(afd, &st)) {
      fprintf(stderr, "ERROR, cannot stat file %s\n", attachmentfile);
      exit(1);
    }
    attachment_len = st.st_size;
    req_len += attachment_len;
  }

  reqs.emplace_back(req_len);

  email_request_info *info;
  info = reinterpret_cast<email_request_info *>(reqs[reqs.size() - 1].data());
  info->req_type = REQ_TYPE_TEXT_AND_ATTACHMENT;
  info->email_text_len = text_len + 1;
  info->attachment_len = attachment_len;
  info->data[text_len] = 0;

  read_file(tfd, (char *)info->data, text_len, textfile);
  close(tfd);
  if (attachmentfile)
    read_file(afd, (char *)info->data + text_len + 1, attachment_len,
              attachmentfile);
  close(afd);
}

void print_result(char *desc, unsigned char *data) {
  struct pipeline_result *piperes;
  piperes = (struct pipeline_result *)data;
  printf("\nresult for %s\n", desc);
  printf("----------- CLAMAV SUMMARY -----------\n");
  printf("Known viruses: %u\n", piperes->cinfo.sigs);
  printf("Scanned files: %u\n", piperes->cinfo.files);
  printf("Infected files: %u\n", piperes->cinfo.ifiles);
  if (piperes->cinfo.errors)
    printf("Total errors: %u\n", piperes->cinfo.errors);
  printf("Data scanned: %u blocks\n", piperes->cinfo.blocks);
  printf("Data read: %u blocks\n", piperes->cinfo.rblocks);

  printf("----------- DSPAM SUMMARY -----------\n");
  printf("Classify result: %s\n", piperes->dinfo.result_str);
}

void *receiver(void *in) {
  int i;
  struct thread_info *ti = (struct thread_info *)in;
  for (i = 0; i < (int)reqs.size() * iters; i++) {
    int not_ready;
    ssize_t dlen, len;
    unsigned char *buf, *data;
    char *desc;
    size_t size = 0;
    size_t end = 0;
    if (ti->io_model) {
      while ((not_ready =
                  get_chan_data_no_ctx(&chan, 1, &buf, &size, ti->enc)) >= 0) {
        if (not_ready) continue;
        if (get_work_desc_buffer(buf, size, 0, &end, &desc, &dlen, &data,
                                 &len)) {
          error("get_work_desc_buffer\n");
        }
        if (end != size) {
          error("ERROR, wrong size");
        }
        print_result(desc, data);
        // reset values
        end = 0;
        size = 0;
        free(buf);
        break;
      }
    } else {
      while ((not_ready = get_work_desc_no_ctx(&chan, 1, &desc, &dlen, &data,
                                               &len)) >= 0) {
        if (not_ready) continue;
        print_result(desc, data);
        free(desc);
        free(data);
        break;
      }
    }
  }
  return NULL;
}

void load_all_files(const char *dir, int n) {
  char buf1[1024];
  char buf2[1024];
  int i;
  for (i = 0; i < n; i++) {
    sprintf(buf1, "%s/%d.txt", dir, i);
    sprintf(buf2, "%s/%d.pdf", dir, i);
    alloc_req_buf(buf1, buf2);
  }
}

int main(int argc, char *argv[]) {
  pthread_t receiver_tid;
  int sockfd, portno, i;

  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[BUFSIZE];
  struct thread_info ti = {0};

  // printf("argc: %d\n", argc);
  if (argc < 7) {
    fprintf(stderr,
            "usage: %s hostname port n_iters io_model input_dir n_files\n",
            argv[0]);
    exit(1);
  }
  iters = atoi(argv[3]);
  ti.io_model = (atoi(argv[4]) == 1);
  load_all_files(argv[5], atoi(argv[6]));

  ti.enc = RyoanCheckShouldEncrypt();

  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(1);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");
  bzero(buffer, BUFSIZE);

  chan = build_plain_text_channel(sockfd, sockfd);

  if (pthread_create(&receiver_tid, NULL, &receiver, &ti)) {
    error("ERROR, pthread_create");
  }

  for (i = 0; i < iters; i++) {
    for (auto it = reqs.begin(); it != reqs.end(); it++) {
      send_req(it->data(), it->size(), sockfd, chan, ti.enc, ti.io_model);
    }
  }

  pthread_join(receiver_tid, NULL);
  return 0;
}
