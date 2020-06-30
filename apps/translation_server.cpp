/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "pipeline/channel.h"
#include "pipeline/pipeline.h"

#define BUFSIZE 0x100000
#define MAXEVENTS 64

const char *prog_name = "email_pipeline_client";

char *pipeline_starter;
char *pipeline_json_file;

int fdin1[2], fdout1[2];
int fdin2[2], fdout2[2];

void error_and_exit(const char *msg) {
  perror(msg);
  exit(1);
}

void make_fd_non_blocking(int sfd) {
  int flags, s;

  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    error_and_exit("fcntl");
  }

  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1) {
    error_and_exit("fcntl");
  }
}

void init_pipeline() {
  pid_t childpid;

  if (pipe(fdin1) || pipe(fdout1) || pipe(fdin2) || pipe(fdout2)) {
    error_and_exit("pipe");
  }

  make_fd_non_blocking(fdin1[0]);
  // make_fd_non_blocking(fdin1[1]);
  // make_fd_non_blocking(fdout1[0]);
  // make_fd_non_blocking(fdout1[1]);
  make_fd_non_blocking(fdin2[0]);
  // make_fd_non_blocking(fdin2[1]);
  // make_fd_non_blocking(fdout2[0]);
  // make_fd_non_blocking(fdout2[1]);
  if ((childpid = fork()) == -1) {
    error_and_exit("fork");
  }

  if (childpid == 0) {
    char inbuf[32];
    char outbuf[32];
    close(fdin1[1]);
    close(fdout1[0]);
    close(fdin2[1]);
    close(fdout2[0]);
    sprintf(inbuf, "--in=%d,%d", fdin1[0], fdout1[1]);
    sprintf(outbuf, "--out=%d,%d", fdin2[0], fdout2[1]);
    execl(pipeline_starter, pipeline_starter, pipeline_json_file, inbuf, outbuf,
          "--out_size_func=a0b350", NULL);
  }
  close(fdin1[0]);
  close(fdout1[1]);
  close(fdin2[0]);
  close(fdout2[1]);
}

void *req_receiver(void *socket) {
  int sock = *(int *)socket;
  int n, m;
  unsigned char buffer[BUFSIZE];
  unsigned char *tmp;
  while (1) {
    n = read(sock, buffer, BUFSIZE);
    if (n < 0) {
      error_and_exit("read from socket\n");
    }
    tmp = buffer;
    while (n > 0) {
      m = write(fdin1[1], tmp, n);
      if (m < 0) {
        error_and_exit("write to input pipe\n");
      }
      n -= m;
      tmp += m;
    }
  }
  return NULL;
}

void *process_req(int socket) {
  int pos = 0;
  int total;
  int size;
  int efd;
  int n, i;
  struct epoll_event event;
  struct epoll_event *events;
  unsigned char buffer[BUFSIZE];

  efd = epoll_create1(0);
  if (efd < 0) {
    error_and_exit("epool_create1\n");
  }

  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fdout1[0];
  if (epoll_ctl(efd, EPOLL_CTL_ADD, fdout1[0], &event)) {
    error_and_exit("epoll_ctl ADD\n");
  }
  event.data.fd = fdout2[0];
  if (epoll_ctl(efd, EPOLL_CTL_ADD, fdout2[0], &event)) {
    error_and_exit("epoll_ctl ADD\n");
  }
  events = (epoll_event *)calloc(MAXEVENTS, sizeof event);

  while (1) {
    n = epoll_wait(efd, events, MAXEVENTS, -1);
    for (i = 0; i < n; ++i) {
      if (((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) &&
          (!(events[i].events & EPOLLIN))) {
        error_and_exit("wait events\n");
      } else {
        total = read(events[i].data.fd, buffer, BUFSIZE);
        if (total < 0) {
          error_and_exit("read from output pipe\n");
        }

        if (total == 0) {
          continue;
        }

        pos = 0;
        while (pos < total) {
          size = write(socket, buffer + pos, total);
          if (size < 0) {
            error_and_exit("write result to socket\n");
          }
          pos += size;
        }
      }
    }
  }
  free(events);
  return NULL;
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  unsigned int clilen;
  char buffer[BUFSIZE];
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t req_receiver_tid;

  if (argc != 4) {
    sprintf(buffer, "usage: %s port pipeline_starter_prog pipeline_json_file\n",
            argv[0]);
    error_and_exit(buffer);
  }
  pipeline_starter = argv[2];
  pipeline_json_file = argv[3];
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) error_and_exit("error_and_exit opening socket");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error_and_exit("error_and_exit on binding");
  listen(sockfd, 5);
  init_pipeline();
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0) error_and_exit("error_and_exit on accept");
  bzero(buffer, BUFSIZE);

  if (pthread_create(&req_receiver_tid, NULL, &req_receiver, &newsockfd)) {
    error_and_exit("ERROR, pthread_create");
  }
  process_req(newsockfd);

  pthread_join(req_receiver_tid, NULL);
  return 0;
}
