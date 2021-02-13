#include "csapp.h"

#ifndef __CACHE_H_
#define __CACHE_H_

#define CACHE_SIZE 9
#define CACHE_BUF_SIZE 1024000

typedef struct cache_block{
    char * uri;
    char*buf;
    int size;
    pthread_rwlock_t rwlock;
}cache_block;

extern cache_block caches[CACHE_SIZE];

void cache_init();
int cache_read(char*uri, int connfd);
void cache_write(char *uri, char*data, int size);
void cache_free();
void print_cache();

#endif /* __CACHE_H_ */
