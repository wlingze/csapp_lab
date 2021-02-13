#include "cache.h"
#include "csapp.h"

cache_block caches[CACHE_SIZE];

void cache_init() {
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    caches[i].buf = Malloc(CACHE_BUF_SIZE);
    caches[i].uri = Malloc(MAXLINE);
    /*strcpy(caches[i].buf, "");*/
    memset(caches[i].buf, 0, CACHE_BUF_SIZE);
    strcpy(caches[i].uri, "");
    caches[i].size = 0;
    pthread_rwlock_init(&caches[i].rwlock, NULL);
  }
  return;
}

int cache_read(char *uri, int connfd) {
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    pthread_rwlock_rdlock(&caches[i].rwlock);
    if (!strcmp(caches[i].uri, uri)) {
      Rio_writen(connfd, caches[i].buf, caches[i].size);
      pthread_rwlock_unlock(&caches[i].rwlock);
      printf("use cache[%d]!\n", i);
      return 1;
    }
    pthread_rwlock_unlock(&caches[i].rwlock);
  }
  return 0;
}

void cache_write(char *uri, char *data, int size) {
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    pthread_rwlock_rdlock(&caches[i].rwlock);
    if (!strcmp(uri, caches[i].uri)) {
      printf("cache[%d] continue write\n", i);
      pthread_rwlock_unlock(&caches[i].rwlock);

      pthread_rwlock_wrlock(&caches[i].rwlock);
      strcpy(caches[i].uri, uri);
      strcat(caches[i].buf, data);
      caches[i].size += size;
      pthread_rwlock_unlock(&caches[i].rwlock);

      return;
    }
    pthread_rwlock_unlock(&caches[i].rwlock);
  }

  for (i = 0; i < CACHE_SIZE; i++) {
    pthread_rwlock_rdlock(&caches[i].rwlock);
    if (!caches[i].size) {
      printf("cache[%d] start write\n", i);
      pthread_rwlock_unlock(&caches[i].rwlock);

      pthread_rwlock_wrlock(&caches[i].rwlock);
      memcpy(caches[i].uri, uri, MAXLINE);
      memcpy(caches[i].buf, data, size);
      caches[i].size += size;
      pthread_rwlock_unlock(&caches[i].rwlock);

      return;
    }
    pthread_rwlock_unlock(&caches[i].rwlock);
  }
  return;
}

void cache_free() {
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    free(caches[i].buf);
    free(caches[i].uri);
    caches[i].size = 0;
    caches[i].buf = 0;
    caches[i].uri = 0;
    pthread_rwlock_destroy(&caches[i].rwlock);
  }
  return;
}

void print_cache() {
  int i;
  for (i = 0; i < CACHE_SIZE; i++) {
    pthread_rwlock_rdlock(&caches[i].rwlock);
    printf("caches[%d]\n"
           "uri: %s\n"
           "data:\n"
           "----\n"
           "%s\n"
           "----\n",
           i, caches[i].uri, caches[i].buf);
    pthread_rwlock_unlock(&caches[i].rwlock);
  }
  return;
}
