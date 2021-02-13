/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WSIZE 8
#define DSIZE 16
#define nil 0

#define CHUNK_SIZE (1 << 12)

team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACT(size, use) ((size) | (use))

#define chunk_index(p, index) ((p) + index)

#define get(p, index) (*(unsigned int *)chunk_index(p, index))
#define put(p, index, val) (get(p, index) = (val))

#define GET(p) (get(p, 0))
#define PUT(p, val) (put(p, 0, val))

#define GETSIZE(p) (GET(p) & ~7)

#define NEXT(p) ((char *)(p) + GETSIZE((p)))
#define PREV(p) ((char *)(p)-GETSIZE((p)-WSIZE))

#define isuse(p) (GET(p) & 1)
#define prev_isuse(p) ((GET(p) & 2) >> 1)

#define CHUNK_INUSE 1
#define PREV_INUSE 2

#define set_inuse(p, size)                                                     \
  {                                                                            \
    put(p, 0, (size | 1 | (prev_isuse(p) << 1)));                              \
    put(p, size, (GET(NEXT(p)) | PREV_INUSE));                                 \
  }

#define inuse(p, size)                                                         \
  {                                                                            \
    unlink(p);                                                                 \
    set_inuse(p, size);                                                        \
  }

#define set_unuse(p, size)                                                     \
  {                                                                            \
    put(p, 0, (size | (prev_isuse(p) << 1)));                                  \
    put(p, size - WSIZE, size);                                                \
    put(p, size, (GET(NEXT(p)) & (~PREV_INUSE)));                              \
  }

#define unuse(p, size)                                                         \
  {                                                                            \
    link(p);                                                                   \
    set_unuse(p, size);                                                        \
  }

#define get(p, index) (*(unsigned int *)chunk_index(p, index))
#define put(p, index, val) (get(p, index) = (val))

#define FD(p) (*((unsigned long *)chunk_index(p, WSIZE)))
#define BK(p) (*((unsigned long *)chunk_index(p, 2 * WSIZE)))

#define link(p)                                                                \
  {                                                                            \
    BK(p) = BK(heap_list);                                                     \
    FD(p) = (unsigned long)heap_list;                                          \
    BK(heap_list) = (unsigned long)p;                                          \
    FD(BK(p)) = (unsigned long)p;                                              \
  }

#define unlink(p)                                                              \
  {                                                                            \
    BK(FD(p)) = BK(p);                                                         \
    FD(BK(p)) = FD(p);                                                         \
    FD(p) = nil;                                                               \
    BK(p) = nil;                                                               \
  }

#define chunk2mem(p) (chunk_index(p, WSIZE))
#define mem2chunk(mem) (chunk_index(mem, -WSIZE))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// #define DEBUG

#ifdef DEBUG
#define dprint printf
#else
#define dprint
#endif

void *heap_list;

static void *extend_heap(int size);
static void *coalesced(void *p);
static void place(void *p, size_t size);
static void *find_fit(size_t size);

static void *extend_heap(int word) {

  void *bp;
  size_t size = (word % 2) ? ((word + 1) * WSIZE) : (word * WSIZE);

  if ((bp = mem_sbrk(size)) == (void *)-1)
    return NULL;

  unuse(bp, size);

  PUT((NEXT(bp)), (CHUNK_INUSE));

  return coalesced(bp);
}

static void *coalesced(void *p) {

  size_t prev_use = prev_isuse(p);
  size_t next_use = isuse((NEXT(p)));
  size_t size = GETSIZE((p));

  if (prev_use && next_use) {
  } else if (prev_use && (!next_use)) {
    unlink(NEXT(p));
    unlink(p);
    size += GETSIZE((NEXT(p)));
    unuse(p, size);
  } else if ((!prev_use) && next_use) {
    unlink(PREV(p));
    unlink(p);
    size += GETSIZE((PREV(p)));
    p = PREV(p);
    unuse(p, size);
  } else if ((!prev_use) && (!next_use)) {
    unlink(PREV(p));
    unlink(NEXT(p));
    unlink(p);
    size += GETSIZE((PREV(p))) + GETSIZE((NEXT(p)));
    p = PREV(p);
    unuse(p, size);
  }
  return p;
}

static void place(void *p, size_t size) {
  size_t psize = GETSIZE((p));
  if ((psize - size) >= (2 * DSIZE)) {
    inuse(p, size);

    unuse(NEXT(p), (psize - size));
  } else {

    inuse(p, psize);
  }
}

static void *find_fit(size_t size) {
  void *tmp;
  tmp = (void *)BK(heap_list);
  while (tmp != heap_list) {
    if (GETSIZE(tmp) >= (unsigned int)(size)) {
      return tmp;
    }
    tmp = (void *)BK(tmp);
  }
  return NULL;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if ((heap_list = mem_sbrk(4 * WSIZE)) == (void *)-1)
    return -1;
  BK(heap_list) = FD(heap_list) = (size_t)heap_list;
  inuse(heap_list, 4 * WSIZE);

  BK(heap_list) = FD(heap_list) = (size_t)heap_list;

  PUT(NEXT(heap_list), (CHUNK_INUSE | PREV_INUSE));

  if (extend_heap(CHUNK_SIZE / WSIZE) == NULL)
    return -1;
  return 0;
}

void *mm_malloc(size_t size) {
  int newsize;
  void *tmp;
  if (heap_list == NULL) {
    mm_init();
  }
  if (size == 0) {
    return NULL;
  }
  if (size <= DSIZE)
    newsize = 2 * DSIZE;
  else
    newsize = WSIZE * ((size + (DSIZE) + (WSIZE - 1)) / WSIZE);
  if ((tmp = find_fit(newsize)) != NULL) {
    place(tmp, newsize);
    return chunk2mem(tmp);
  }
  size_t extend_size = MAX(newsize, CHUNK_SIZE);
  if ((tmp = extend_heap(extend_size / WSIZE)) == NULL)
    return NULL;
  place(tmp, newsize);
  return chunk2mem(tmp);
}

void mm_free(void *ptr) {
  if (ptr == 0)
    return;
  void *tmp = mem2chunk(ptr);
  size_t size = GETSIZE((tmp));
  unuse(tmp, size);
  coalesced(tmp);
}

void *mm_realloc(void *ptr, size_t size) {

  void *p;
  size_t copy_size;

  if (ptr == NULL) {
    p = mm_malloc(size);
    return p;
  }

  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  copy_size = GETSIZE(mem2chunk(ptr));

  p = mm_malloc(size);

  if (!p) {
    return NULL;
  }

  if (size < copy_size)
    copy_size = size;

  memcpy(p, ptr, copy_size);
  mm_free(ptr);

  return p;
}
