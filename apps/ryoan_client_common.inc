#include <vector>
#include <cstdio>
#include <cstdlib>

std::vector<std::vector<char>> reqs;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int RyoanCheckShouldEncrypt() {
  const char *ryoan_no_enc = getenv("RYOAN_NO_ENCRYPT");
  if (ryoan_no_enc) {
    switch (ryoan_no_enc[0]) {
      case 'y':
      case 'Y':
      case 't':
      case 'T':
      case '1':
        printf("Disabling input encryption\n");
        return 0;
    }
  }
  return 1;
}

static uint64_t req_cnt = 0;

void send_req(const void *buffer, int size, int sock, channel_t *chan, bool enc,
              bool io_model) {
  char buf[32];
  sprintf(buf, "%ld", req_cnt++);
  if (io_model) {
    ssize_t total = 1;
    if (write(sock, &total, sizeof(ssize_t)) != sizeof(ssize_t)) {
      perror("failed to write num requests");
      return;
    }
    if (feed_data_desc_chan_no_spec(&chan, 1, buf, strlen(buf) + 1, buffer,
                                    size, enc)) {
      error("feed data to nacl\n");
    }
  } else {
    if (feed_data_desc_chan_no_model(chan, buf, strlen(buf) + 1, buffer,
                                     size)) {
      error("feed data to next stage\n");
    }
  }
}
