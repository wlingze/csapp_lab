#include "cache.h"
#include "csapp.h"
#include <stdio.h>

/* Recommended max cache and object sizes */
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*void *thread(void *vargp);*/
void proxy(int connfd);
void parse_clien(char *uri, char *host, char *port, char *filename);
void do_proxy(int connfd, int clientfd, char *buf, char *uri);
void do_listen(int connfd, char *uri);

void *thread(void *vargp) {
  int connfd = *(int *)vargp;
  Pthread_detach(pthread_self());
  Free(vargp);
  proxy(connfd);
  Close(connfd);
  return NULL;
}

int main(int argc, char *argv[]) {
  int listenfd;
  int *connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE];
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  cache_init();
  printf("init cache!\n");
  listenfd = Open_listenfd(argv[1]);
  clientlen = sizeof(struct sockaddr_storage);

  while (1) {
    connfd = (int *)(Malloc(sizeof(int)));
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connect to (%s, %s)\n", client_hostname, client_port);
    Pthread_create(&tid, NULL, thread, connfd);
  }
  cache_free();
  printf("free cache!\n");
  exit(0);
}

void proxy(int connfd) {

  int clientfd;
  char uri[MAXLINE], buf[MAXLINE];
  char host[MAXLINE], port[MAXLINE], filename[MAXLINE];

  do_listen(connfd, uri);
  printf("URI: %s\n", uri);
  strcpy(buf, uri);

  if (cache_read(uri, connfd)) {
    return;
  }

  parse_clien(buf, host, port, filename);
  printf("Host: %s\n", host);
  printf("Port: %s\n", port);
  printf("Filename: %s\n", filename);

  sprintf(buf,
          "GET %s HTTP/1.0\r\n"
          "Host: %s:%s\r\n"
          "%s"
          "Accept: */*\r\n"
          "Connection: close\r\n"
          "Proxy-Connection: close\r\n"
          "\r\n",
          filename, host, port, user_agent_hdr);

  clientfd = Open_clientfd(host, port);
  printf("Connect to (%s, %s)\n", host, port);

  do_proxy(connfd, clientfd, buf, uri);

  Close(clientfd);
  printf("Close connect to (%s, %s)\n", host, port);
}

void parse_clien(char *uri, char *host, char *port, char *filename) {
  char *tmp, *tmp2;
  tmp = strtok(uri, "/");
  tmp = strtok(NULL, "/");
  tmp2 = strtok(NULL, "/");

  tmp = strtok(tmp, ":");
  strcpy(host, tmp);

  tmp = strtok(NULL, ":");
  strcpy(port, tmp);

  strcpy(filename, "/");
  strcat(filename, tmp2);

  return;
}

void do_proxy(int connfd, int clientfd, char *buf, char *uri) {

  rio_t rio;
  int n;

  Rio_readinitb(&rio, clientfd);
  Rio_writen(clientfd, buf, strlen(buf));
  while ((n = Rio_readlineb(&rio, buf, MAXLINE))) {
    Rio_writen(connfd, buf, n);
    cache_write(uri, buf, n);
    printf("write cache!\n");
  }
  print_cache();
}

void do_listen(int connfd, char *uri) {
  rio_t rio;
  char buf[MAXLINE];
  char *tmp;

  Rio_readinitb(&rio, connfd);
  if (!Rio_readlineb(&rio, buf, MAXLINE))
    return;
  printf("%s", buf);

  tmp = strtok(buf, " ");
  tmp = strtok(NULL, " ");
  strcpy(uri, tmp);

  return;
}
