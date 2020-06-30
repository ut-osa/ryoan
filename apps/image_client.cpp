#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pipeline/pipeline.h>
#include <pipeline/worker.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "image_request.h"

#include "ryoan_client_common.inc"

using namespace std;

const char *prog_name = "image_pipeline_client";

int portno;
struct sockaddr_in serv_addr;
bool enc;

int64_t req_len = 0;

int64_t iters = 1;

void make_fd_non_blocking(int sfd) {
  int flags, s;

  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    error("fcntl");
  }

  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1) {
    error("fcntl");
  }
}

void load_input_file(const char *dirname, const char *fname) {
  std::string full_name(dirname);
  full_name += fname;
  int fd = open(full_name.c_str(), O_RDONLY);
  struct stat filestat;
  if (fd == -1) error("couldn't open file");
  if (fstat(fd, &filestat)) {
    error("fstat");
  }
  reqs.emplace_back(filestat.st_size);
  uint8_t *data = reinterpret_cast<uint8_t *>(reqs[reqs.size() - 1].data());
  if (read(fd, data, filestat.st_size) != filestat.st_size) {
    error("read");
  }
}

uintptr_t count;
void *receiver(channel_t *chan) {
  int not_ready;
  ssize_t dlen, len;
  unsigned char *buf, *data;
  char *desc;
  size_t size = 0;
  size_t end = 0;

  while ((not_ready = get_chan_data_no_ctx(&chan, 1, &buf, &size, enc)) >= 0) {
    if (not_ready) continue;
    if (get_work_desc_buffer(buf, size, 0, &end, &desc, &dlen, &data, &len)) {
      error("get_work_desc_buffer\n");
    }
    if (end != size) {
      error("ERROR, wrong size");
    }
    printf("%s\n", desc);
    // reset values
    end = 0;
    size = 0;
    free(buf);
    if (!--count) break;
  }

  return NULL;
}

int connect_server() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");
  if (connect(sockfd, (struct sockaddr *)&serv_addr,
              sizeof(struct sockaddr_in)) < 0)
    error("ERROR connecting");
  return sockfd;
}

void send_all() {
  count = reqs.size();
  int sock = connect_server();
  channel_t *chan = build_plain_text_channel(sock, sock);
  for (const auto &req : reqs) {
    send_req(req.data(), req.size(), sock, chan, enc, true);
  }
  receiver(chan);
  free_channel(chan, 1);
}

int main(int argc, char *argv[]) {
  int i;
  struct hostent *server;
  struct dirent *entry;
  DIR *dir;

  if (argc < 5) {
    fprintf(stderr, "usage %s hostname port n_iters input_dir\n", argv[0]);
    exit(1);
  }
  server = gethostbyname(argv[1]);
  portno = atoi(argv[2]);
  iters = atoi(argv[3]);
  dir = opendir(argv[4]);
  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }
  enc = RyoanCheckShouldEncrypt();
  while ((entry = readdir(dir)) != NULL) {
    switch (entry->d_type) {
      case DT_DIR:
        if (!strcmp(entry->d_name, ".")) break;
        if (!strcmp(entry->d_name, "..")) break;
      case DT_BLK:
      case DT_UNKNOWN:
        fprintf(stderr, "fail to open: %s\n", entry->d_name);
        break;
      default:
        load_input_file(argv[4], entry->d_name);
    }
  }
  closedir(dir);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(1);
  }

  printf("Connection started\n");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);

  for (i = 0; i < iters; i++) {
    send_all();
  }
  return 0;
}
