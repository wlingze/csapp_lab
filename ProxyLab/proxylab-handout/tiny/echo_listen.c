#include "csapp.h"

void echo(int connfd);

void sigchld_handle(int sig) {
  while (waitpid(-1, 0, WNOHANG) > 0) {
  }
  return;
}

int main(int argc, char *argv[]) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  Signal(SIGCHLD, sigchld_handle);
  listenfd = Open_listenfd(argv[1]);
  clientlen = sizeof(struct sockaddr_storage);

  while (1) {
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (Fork() == 0) {
      Close(listenfd);
      echo(connfd);
      Close(connfd);
      exit(0);
    }
    Close(connfd);
  }
  exit(0);
}

void echo(int connfd) {
  size_t n;
  char buf[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, connfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    printf("server received %d bytes\n", (int)n);
    printf("%s\n", buf);
    Rio_writen(connfd, buf, n);
  }
}
