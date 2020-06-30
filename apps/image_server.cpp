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
#include <queue>

#include "pipeline/channel.h"
#include "pipeline/pipeline.h"

#define BUFSIZE 0x500000
#define MAXEVENTS 64

#define NWORKERS 1
pthread_t workers[NWORKERS];
pthread_mutex_t global_mtx;
pthread_cond_t global_cv;
std::queue<int> global_q;

const char *prog_name = "image_pipeline_server";

char *pipeline_starter;
char *pipeline_json_file;

struct per_worker_queue {
  std::queue<int> q;
  pthread_mutex_t mtx;
  pthread_cond_t cv;

  int fdin1[2];
  int fdout1[2];
  int fdin2[2];
  int fdout2[2];
};

void error_and_exit(const char *msg) {
  if (errno)
    perror(msg);
  else
    fprintf(stderr, "%s\n", msg);
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

void init_pipeline(struct per_worker_queue *q) {
  pid_t childpid;

  pthread_mutex_init(&q->mtx, NULL);
  pthread_cond_init(&q->cv, NULL);
  if (pipe(q->fdin1) || pipe(q->fdout1) || pipe(q->fdin2) || pipe(q->fdout2)) {
    error_and_exit("pipe");
  }

  make_fd_non_blocking(q->fdin1[0]);
  // make_fd_non_blocking(q->fdin1[1]);
  // make_fd_non_blocking(q->fdout1[0]);
  // make_fd_non_blocking(q->fdout1[1]);
  make_fd_non_blocking(q->fdin2[0]);
  // make_fd_non_blocking(q->fdin2[1]);
  // make_fd_non_blocking(q->fdout2[0]);
  // make_fd_non_blocking(q->fdout2[1]);
  if ((childpid = fork()) == -1) {
    error_and_exit("fork");
  }

  if (childpid == 0) {
    char inbuf[32];
    char outbuf[32];
    close(q->fdin1[1]);
    close(q->fdout1[0]);
    close(q->fdin2[1]);
    close(q->fdout2[0]);
    sprintf(inbuf, "--in=%d,%d", q->fdin1[0], q->fdout1[1]);
    sprintf(outbuf, "--out=%d,%d", q->fdin2[0], q->fdout2[1]);
    execl(pipeline_starter, pipeline_starter, pipeline_json_file, inbuf, outbuf,
          "--out_size_func=a1.0b800", NULL);
  }
  close(q->fdin1[0]);
  close(q->fdout1[1]);
  close(q->fdin2[0]);
  close(q->fdout2[1]);
}

void read_n_bytes(int fd, void *buf, int n, const char *fail_msg) {
  int m;
  char *tmp = (char *)buf;
  while (n > 0) {
    m = read(fd, tmp, n);
    if (m < 0) {
      error_and_exit(fail_msg);
    }
    n -= m;
    tmp += m;
  }
}
bool native;

void *req_receiver(void *per_worker_q) {
  struct per_worker_queue *q = (struct per_worker_queue *)per_worker_q;
  int i, sock, n, m;
  char buffer[BUFSIZE];
  pthread_mutex_lock(&global_mtx);
  while (global_q.empty()) {
    pthread_cond_wait(&global_cv, &global_mtx);
  }
  sock = global_q.front();
  global_q.pop();
  pthread_mutex_unlock(&global_mtx);

  while (1) {
    ssize_t total_items;
    char *tmp = buffer;
    read_n_bytes(sock, &total_items, sizeof(total_items),
                 "read total items from socket\n");
    if (native) {
      assert(total_items == 1);
    } else {
      *(ssize_t *)tmp = total_items;
      tmp += sizeof(total_items);
    }
    for (i = 0; i < total_items; i++) {
      ssize_t trans_size;
      read_n_bytes(sock, &trans_size, sizeof(trans_size),
                   "read transimission size from socket\n");
      if (native) {
        size_t trash;
        read_n_bytes(sock, &trash, sizeof(size_t), "strip extra long");
        trans_size -= sizeof(size_t);
      } else {
        *(ssize_t *)tmp = trans_size;
        tmp += sizeof(trans_size);
      }
      while (trans_size > 0) {
        n = read(sock, tmp, trans_size);
        if (n < 0) {
          error_and_exit("read data from socket\n");
        }
        trans_size -= n;
        tmp += n;
      }
    }
    n = tmp - buffer;
    tmp = buffer;
    while (n > 0) {
      m = write(q->fdin1[1], tmp, n);
      if (m < 0) {
        error_and_exit("write to input pipe\n");
      }
      n -= m;
      tmp += m;
    }

    pthread_mutex_lock(&q->mtx);
    q->q.push(sock);
    pthread_cond_signal(&q->cv);
    pthread_mutex_unlock(&q->mtx);
  }
  return NULL;
}

void *run_pipeline(void *arg) {
  char buffer[BUFSIZE];
  pthread_t req_receiver_tid;
  struct per_worker_queue *q = new struct per_worker_queue;
  if (!q) {
    error_and_exit("malloc per_worker_queue\n");
  }

  init_pipeline(q);
  if (pthread_create(&req_receiver_tid, NULL, &req_receiver, q)) {
    error_and_exit("ERROR, pthread_create for request receiver\n");
  }

  int sock;
  while (1) {
    int i;
    ssize_t total_items;
    ssize_t trans_size;
    char *tmp = buffer;
    int n, m;
    pthread_mutex_lock(&q->mtx);
    while (q->q.empty()) {
      pthread_cond_wait(&q->cv, &q->mtx);
    }
    sock = q->q.front();
    q->q.pop();
    pthread_mutex_unlock(&q->mtx);
    if (native) {
      ssize_t *trans_size_ptr;
      ssize_t *trans_size_ptr_2;
      ssize_t dlen, len;

      *(ssize_t *)tmp = 1;
      tmp += sizeof(ssize_t);

      trans_size_ptr = (ssize_t *)tmp;
      tmp += sizeof(ssize_t);
      trans_size_ptr_2 = (ssize_t *)tmp;
      tmp += sizeof(ssize_t);

      *trans_size_ptr = sizeof(ssize_t);
      *trans_size_ptr_2 = 0;

      /* desc */
      read_n_bytes(q->fdout2[0], &dlen, sizeof(dlen),
                   "getting transmission size from pipe\n");
      *(ssize_t *)tmp = dlen;
      *trans_size_ptr += dlen + sizeof(dlen);
      *trans_size_ptr_2 += dlen + sizeof(dlen);
      tmp += sizeof(dlen);

      while (dlen > 0) {
        m = read(q->fdout2[0], tmp, dlen);
        if (m < 0) {
          error_and_exit("reading data from pipe\n");
        }
        tmp += m;
        dlen -= m;
      }

      /*data */
      read_n_bytes(q->fdout2[0], &len, sizeof(len),
                   "getting transmission size from pipe\n");
      *(ssize_t *)tmp = len;
      *trans_size_ptr += len + sizeof(len);
      *trans_size_ptr_2 += len + sizeof(len);
      tmp += sizeof(len);

      while (len > 0) {
        m = read(q->fdout2[0], tmp, len);
        if (m < 0) {
          error_and_exit("reading data from pipe\n");
        }
        tmp += m;
        len -= m;
      }
    } else {
      read_n_bytes(q->fdout2[0], &total_items, sizeof(total_items),
                   "getting total items from pipe\n");
      *(ssize_t *)tmp = total_items;
      tmp += sizeof(total_items);

      for (i = 0; i < total_items; ++i) {
        read_n_bytes(q->fdout2[0], &trans_size, sizeof(trans_size),
                     "getting transmission size from pipe\n");
        *(ssize_t *)tmp = trans_size;
        tmp += sizeof(trans_size);

        while (trans_size > 0) {
          m = read(q->fdout2[0], tmp, trans_size);
          if (m < 0) {
            error_and_exit("reading data from pipe\n");
          }
          tmp += m;
          trans_size -= m;
        }
      }
    }

    n = tmp - buffer;
    tmp = buffer;

    printf("SENDING TO CLIENT\n");
    while (n > 0) {
      m = write(sock, tmp, n);
      if (m < 0) {
        error_and_exit("write data to socket\n");
      }
      tmp += m;
      n -= m;
    }
  }
  close(sock);
  pthread_join(req_receiver_tid, NULL);
  delete q;
  return NULL;
}

void enqueue(int sock) {
  pthread_mutex_lock(&global_mtx);
  global_q.push(sock);
  pthread_cond_signal(&global_cv);
  pthread_mutex_unlock(&global_mtx);
}

bool decide_native(int *argc, char *argv[]) {
  int i;
  for (i = 0; i < *argc; i++) {
    if (strcmp(argv[i], "--native") == 0) {
      for (; i < *argc - 1; i++) {
        argv[i] = argv[i + 1];
      }
      *argc -= 1;
      return true;
    }
  }
  return false;
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  int i;
  unsigned int clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;

  pthread_mutex_init(&global_mtx, NULL);
  pthread_cond_init(&global_cv, NULL);

  if (argc < 4) {
    sprintf(
        buffer,
        "usage: %s port [--native] pipeline_starter_prog pipeline_json_file\n",
        argv[0]);
    error_and_exit(buffer);
  }
  native = decide_native(&argc, argv);
  pipeline_starter = argv[2];
  pipeline_json_file = argv[3];
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) error_and_exit("error_and_exit opening socket");

  for (i = 0; i < NWORKERS; i++) {
    int err = pthread_create(&workers[i], NULL, &run_pipeline, NULL);
    if (err != 0)
      printf("n't create thread :[%s]", strerror(err));
    else
      printf("Worker thread %d created successfully\n", i);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error_and_exit("error_and_exit on binding");
  listen(sockfd, 5);
  while (1) {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) error_and_exit("ERROR on accept");
    enqueue(newsockfd);
  }

  for (i = 0; i < NWORKERS; i++) {
    pthread_join(workers[i], NULL);
  }
  return 0;
}
