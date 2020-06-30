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
#include <fstream>
#include <iostream>
#include <string>
#include "health_request.h"

#include "ryoan_client_common.inc"

const char *prog_name = "health_pipeline_client";

struct sockaddr_in serv_addr;
bool enc;

void load_input_file(const char *fname) {
  std::string line;
  std::ifstream f(fname);
  int disease_id;
  if (!f.is_open()) {
    error("cannot open input file\n");
  }
  while (1) {
    f >> disease_id;
    if (!f.good()) {
      break;
    }
    if (!getline(f, line)) {
      break;
    }
    reqs.emplace_back(sizeof(health_request_info) + line.length() + 1);
    health_request_info *info;
    info =
        reinterpret_cast<health_request_info *>(reqs[reqs.size() - 1].data());
    info->disease_id = disease_id;
    info->data_len = line.length() + 1;
    memcpy(info->data, line.c_str(), line.length() + 1);
  }

  f.close();
}

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
    printf("\nresult for %s: %s\n", desc, data);
    // reset values
    end = 0;
    size = 0;
    free(buf);
    break;
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
  for (auto it = reqs.begin(); it != reqs.end(); it++) {
    int sock = connect_server();
    channel_t *chan = build_plain_text_channel(sock, sock);
    send_req(it->data(), it->size(), sock, chan, enc, true);
    receiver(chan);
    free_channel(chan, 1);
  }
}

int main(int argc, char *argv[]) {
  int portno;
  int i;
  int64_t iters = 1;
  struct hostent *server;

  if (argc < 5) {
    fprintf(stderr, "usage %s hostname port n_iters input_file\n", argv[0]);
    exit(1);
  }
  server = gethostbyname(argv[1]);
  portno = atoi(argv[2]);
  iters = atoi(argv[3]);
  load_input_file(argv[4]);

  enc = RyoanCheckShouldEncrypt();

  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(1);
  }

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
