/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<pthread.h>
#include<queue>

#define BUFSIZE (4096 << 4)
#define NWORKERS 5
pthread_t tid[NWORKERS];
pthread_mutex_t mtx;
pthread_cond_t cv;

std::queue<int> req_q;
char *worker_program;
char *worker_datafile;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void* process_req(void*) {
  int socket;
  pid_t childpid;
  int fdin[2], fdout[2];
  char buffer[BUFSIZE];
  int n;
  //char string[] = "Hello, world!\n";
  //char readbuffer[80];

  if (pipe(fdin) || pipe(fdout)) {
    error("pipe");
  }

  /////////////////////
  // This is fake pipeline setup, should be replaced
  if((childpid = fork()) == -1)
  {
    error("fork");
  }

  if (childpid == 0) {
    close(fdin[1]);
    close(fdout[0]);
    dup2(fdin[0], 0);
    dup2(fdout[1], 1);
    execl(worker_program, worker_program, worker_datafile, NULL);
  }
  // end of pipeline setup

  close(fdin[0]);
  close(fdout[1]);
  while (1) {
    pthread_mutex_lock(&mtx);
    while (req_q.empty()) {
      pthread_cond_wait(&cv, &mtx);
    }
    socket = req_q.front();
    req_q.pop();
    pthread_mutex_unlock(&mtx);

    n = read(socket,buffer,BUFSIZE-1);
    if (n < 0) error("ERROR reading from socket");
    n = write(fdin[1],buffer,n);
    if (n < 0) error("ERROR writing to pipe");
    n = read(fdout[0],buffer,BUFSIZE-1);
    if (n < 0) error("ERROR reading from pipe");
    n = write(socket,buffer,n);
    if (n < 0) error("ERROR writing to socket");
    close(socket);
  }
  return NULL;
}

void enqueue(int sock) {
  pthread_mutex_lock(&mtx);
  req_q.push(sock);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     unsigned int clilen;
     char buffer[BUFSIZE];
     struct sockaddr_in serv_addr, cli_addr;

     if (argc != 4) {
       sprintf(buffer, "usage: %s port worker_program worker_datafile", argv[0]);
       error(buffer);
     }
     worker_program = argv[2];
     worker_datafile = argv[3];

     pthread_mutex_init(&mtx, NULL);
     pthread_cond_init (&cv, NULL);
     for (int i = 0; i < NWORKERS; i++)
     {
       int err = pthread_create(&(tid[i]), NULL, &process_req, NULL);
       if (err != 0)
         printf("n't create thread :[%s]", strerror(err));
       else
         printf("\n Thread created successfully\n");
     }
     //int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     while (1) {
       clilen = sizeof(cli_addr);
       newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       if (newsockfd < 0) 
         error("ERROR on accept");
       bzero(buffer,BUFSIZE);
       enqueue(newsockfd);
     }
     return 0; 
}
