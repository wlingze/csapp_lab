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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define TOPSIZE (1 << 20)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define chunk_index(p, index) ((size_t *)((p) + index))
#define get(p, index) (*(size_t *)chunk_index(p, index))
#define put(p, index, val) ((*(size_t *)(chunk_index(p, index))) = val)

#define chunk2mem(p) (chunk_index(p, 8))
#define mem2chunk(mem) (chunk_index(mem, -8))

#define getsize(p) (get(p, 0) & ~0x1)

#define inuse(p, size)                                                         \
  {                                                                            \
    put(p, 0, size | 1);                                                       \
    put(p, 8 + size, size | 1);                                                \
  }

#define top_reset() (unuse(top_chunk, top_size))

#define unuse(p, size)                                                         \
  {                                                                            \
    put(p, 0, size);                                                           \
    put(p, 8 + size, size);                                                    \
  }

#define isuse(p) (get(p, 0) & 1)

#define next(p) (chunk_index(p, getsize(p) + 0x10))
#define prev(p) (chunk_index(p, -(getsize(chunk_index(p, -8)) + 0x10)))

#define DEBUG 1

#ifdef DEBUG
#define dprint(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprint(...)
#endif

/*chunk struct {
 *
 *  head [use];     8bit
 *  ....
 *  foot [use];     8bit
 * }
 *
 */

static void *top_chunk;
static int top_size;
static void *heap_list;

static void coalesced();
static void *find_fit(int size);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  top_chunk = mem_sbrk(TOPSIZE);
  top_size = TOPSIZE - 0x10;
  top_reset();

  heap_list = top_chunk;
  top_size -= 0x10 + 2 * ALIGNMENT;
  top_chunk += 0x10 + 2 * ALIGNMENT;
  top_reset();
  inuse(heap_list, 0x10);

  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  /*void *p = mem_sbrk(ALIGN(size + 2 * SIZE_T_SIZE));*/

  if ((size <= 0)) {
    return NULL;
  }

  if (size >= top_size) {
    top_chunk = mem_sbrk(TOPSIZE);
    top_size = TOPSIZE - 0x10;
    top_reset();
  }
  int newsize = ALIGN(size);

  dprint("malloc %d\n", newsize);

  void *p = find_fit(size);
  if (p == 0) {
    p = top_chunk;
    top_size -= newsize + 2 * ALIGNMENT;
    top_chunk += newsize + 2 * ALIGNMENT;
    top_reset();
  }
  inuse(p, newsize);
  return (void *)(chunk2mem(p));
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {

  dprint("free %p\n", ptr);

  void *p = mem2chunk(ptr);
  if (!isuse(p)) {
    return;
  }
  unuse(p, getsize(p));
  coalesced(p);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

static void coalesced(void *p) {
  int prev_use = isuse(prev(p));
  int next_use = isuse(next(p));

  if (prev_use & next_use) {
    // both use
    return;
  } else if (prev_use & !next_use) {
    // prev use
    // next coalesced
    void *next = next(p);
    int size = getsize(p) + getsize(next);
    unuse(p, size);

    if (next == top_chunk) {
      top_chunk = p;
      top_size = top_size + size;
    }

    return;
  } else if ((!prev_use) & next_use) {
    // next use
    // prev coaleseced
    void *prev = prev(p);
    int size = getsize(prev(p)) + getsize(p);
    unuse(prev, size);
    return;
  } else if (!prev_use & !next_use) {
    // no use
    void *prev = prev(p);
    void *next = next(p);
    int size = getsize(prev) + getsize(p) + getsize(next);
    unuse(prev, size);

    if (next == top_chunk) {
      top_chunk = prev;
      top_size = getsize(top_chunk);
    }
    return;
  }

  return;
}

static void *find_fit(int size) {
  void *tmp = heap_list;
  dprint("find fit %x[\n", size);
  while (tmp != top_chunk) {
    dprint("tmp: -> %p [0x%lx]\n", tmp, getsize(tmp));
    if (!isuse(tmp) && (getsize(tmp) >= size)) {
      if (getsize(tmp) <= size + 0x10) {
        return tmp;
      }
      int remain_size = getsize(tmp) - size - 0x10;
      unuse(tmp, size);
      void *remain_chunk = chunk_index(tmp, size);
      unuse(remain_chunk, remain_size);
      return tmp;
    }
    tmp = next(tmp);
  }
  dprint("]not found %p\n", tmp);
  return NULL;
}
