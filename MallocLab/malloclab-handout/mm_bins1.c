/*
 * Perf index = 48 (util) + 40 (thru) = 88/100
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

#define get(p, index) (*(size_t *)chunk_index(p, index))
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
  { set_inuse((p), (size)); }

#define set_unuse(p, size)                                                     \
  {                                                                            \
    put(p, 0, (size | (prev_isuse(p) << 1)));                                  \
    put(p, size - WSIZE, size);                                                \
    put(p, size, (GET(NEXT(p)) & (~PREV_INUSE)));                              \
  }

#define unuse(p, size)                                                         \
  {                                                                            \
    set_unuse(p, size);                                                        \
    link(p);                                                                   \
  }

#define FD(p) (*(size_t *)pFD(p))
#define BK(p) (*(size_t *)pBK(p))

#define pFD(p) (((void *)chunk_index(p, WSIZE)))
#define pBK(p) (((void *)chunk_index(p, 2 * WSIZE)))

#define link(p)                                                                \
  {                                                                            \
    void *__header = header_by_point(p);                                       \
    FD(p) = link_first(__header);                                              \
    link_first_push(__header, p);                                              \
  }

#define unlink(p)                                                              \
  {                                                                            \
    void *__header = (void *)header_by_point(p);                               \
    void *__tmp = (void *)link_first(__header);                                \
    if (__tmp == p) {                                                          \
      link_first_pop(__header, FD(p));                                         \
    } else {                                                                   \
      while ((void *)FD(__tmp) != p) {                                         \
        __tmp = (void *)FD(__tmp);                                             \
      }                                                                        \
      FD(__tmp) = FD(p);                                                       \
      PUT(__header,                                                            \
          (link_first(__header) | ((link_len(__header) - 1) & 0x7)));          \
    }                                                                          \
  }

#define chunk2mem(p) (chunk_index(p, WSIZE))
#define mem2chunk(mem) (chunk_index(mem, -WSIZE))

// fast bin
/*#define fastbin_index(size) ((((size) >> 4) - 2) * WSIZE)*/
#define bins_index(size)                                                       \
  {                                                                            \
    int __index = 0;                                                           \
    size_t __size = size;                                                      \
    while (__size > 10) {                                                      \
      __index += 1;                                                            \
      __size >>= 1;                                                            \
      if (__index > 9) {                                                       \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    __index = (__index - 1) * WSIZE;                                           \
  }

#define bin_link_header(size) ((void *)chunk_index(bins, (bins_index(size))))

#define header_by_point(p) (bin_link_header(GETSIZE(p)))
#define header_by_size(size) (bin_link_header(size))

#define link_first_pop(pfast, p)                                               \
  (PUT(pfast, ((size_t)p | ((link_len(pfast) - 1) & 0x7))))
#define link_first_push(pfast, p)                                              \
  (PUT(pfast, ((size_t)p | ((link_len(pfast) + 1) & 0x7))))

#define link_first(p) (GET(p) & ~7)
#define link_len(p) (GET(p) & 7)

#define next_link(header) ((void *)chunk_index(header, WSIZE))
#define last_link (bins + 9 * WSIZE)
#define prev_link(header) ((void *)chunk_index(header, -WSIZE))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*#define DEBUG*/

#ifdef DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#define CHECK() mm_bins_check()
#else
#define DPRINT(...)
#define CHECK()
#endif

void *bins;

static void *extend_heap(int size);
static void *coalesced(void *p);
static void place(void *p, size_t size);
static void *find_fit(size_t size);
static void *realloc_coalesced(void *tmp, size_t newsize, size_t oldsize);
static void *realloc_place(void *tmp, size_t newsize, size_t oldsize);
static void bin_coalesced(size_t size);
static void mm_bins_check();

/*static void unlink(void *p) {*/
/*void *header, *tmp, *first;*/
/*header = header_by_point(p);*/
/*tmp = first = link_first(header);*/
/*if (tmp == p) {*/
/*link_first_pop(header, p);*/
/*} else {*/
/*while (FD(tmp) != p) {*/
/*tmp = FD(tmp);*/
/*}*/
/*}*/

/*return;*/
/*}*/

static void mm_bins_check() {
  void *header, *tmp;
  header = next_link(bins);
  size_t size = 0x10;
  while (header <= last_link) {
    DPRINT("  || bin(%ld) [0x%lx, 0x%lx)\n", link_len(header), size,
           (size << 1));
    tmp = (void *)link_first(header);
    while (tmp != 0) {
      DPRINT("   |\\-> [%p](0x%lx)\n", tmp, GETSIZE(tmp));
      tmp = (void *)FD(tmp);
    }
    size = size << 1;
    header = next_link(header);
  }

  return;
}

static void *extend_heap(int word) {

  void *bp;
  size_t size = (word % 2) ? ((word + 1) * WSIZE) : (word * WSIZE);

  if ((bp = mem_sbrk(size)) == (void *)-1)
    return NULL;

  size_t a = (bins_index(size));
  unuse(bp, size);

  PUT((NEXT(bp)), (CHUNK_INUSE));

  return coalesced(bp);
}

/*static void bin_coalesced(size_t size) {*/
/*void *tmp, *fast;*/

/*fast = header_by_size(size);*/
/*tmp = (void *)link_first(fast);*/
/*while (tmp != NULL) {*/
/*coalesced(tmp);*/
/*tmp = (void *)FD(tmp);*/
/*}*/
/*CHECK();*/
/*return;*/
/*}*/

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
  DPRINT("coalesced \n");
  CHECK();
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
  void *header, *old_header;
  header = header_by_size(size);

  if (link_first(header) != 0) {
    tmp = (void *)link_first(header);
    // find chunk
    while (size > GETSIZE(tmp)) {
      if (FD(tmp) == 0) {
        break;
      }
      tmp = (void *)FD(tmp);
    }
    if (size <= GETSIZE(tmp)) {
      unlink(tmp);
      return tmp;
    }
  }

  // find link
  old_header = header;
  header = last_link;
  while (link_first(header) == 0) {
    if (header == old_header) {
      break;
    }
    header = prev_link(header);
  }
  if (link_first(header) != 0) {
    tmp = (void *)link_first(header);
    // find chunk
    while (size > GETSIZE(tmp)) {
      if (FD(tmp) == 0) {
        break;
      }
      tmp = (void *)FD(tmp);
    }
    if (size <= GETSIZE(tmp)) {
      unlink(tmp);
      return tmp;
    }
  }
  return NULL;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if ((bins = mem_sbrk((1 + 9) * WSIZE)) == (void *)-1)
    return -1;
  set_inuse(bins, (1 + 9) * WSIZE);
  PUT(NEXT(bins), (CHUNK_INUSE | PREV_INUSE));
  memset((bins + WSIZE), 0, 9 * WSIZE);

  if (extend_heap(CHUNK_SIZE / WSIZE) == NULL)
    return -1;
  return 0;
}

void *mm_malloc(size_t size) {
  int newsize;
  void *tmp;
  if (bins == NULL) {
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
  if ((tmp = extend_heap(extend_size / WSIZE)) == NULL) {
    return NULL;
  }

  unlink(tmp);
  place(tmp, newsize);

  DPRINT("malloc(0x%x) [%p]\n", newsize, tmp);
  CHECK();

  return chunk2mem(tmp);
}

void mm_free(void *ptr) {
  if (ptr == 0)
    return;
  void *tmp = mem2chunk(ptr);
  size_t size = GETSIZE((tmp));
  /*if (link_len(header_by_size(size)) > 6) {*/
  /*bin_coalesced(GETSIZE(tmp));*/
  /*}*/

  unuse(tmp, size);
  coalesced(tmp);

  DPRINT("fastbin free[%p](0x%lx)\n", tmp, size);
  CHECK();
}

void *mm_realloc(void *ptr, size_t size) {

  DPRINT("\nrealloc [%p](0x%lx) -> (0x%lx)\n", mem2chunk(ptr),
         GETSIZE(mem2chunk(ptr)), size);
  /*chunk_check();*/
  /*list_check();*/

  size_t oldsize, newsize;
  void *tmp, *p, *newptr;

  if (ptr == NULL) {
    p = mm_malloc(size);
    return p;
  }

  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  tmp = mem2chunk(ptr);
  oldsize = GETSIZE(tmp);
  /*newsize = size + WSIZE;*/

  if (size <= DSIZE)
    newsize = 2 * DSIZE;
  else
    newsize = WSIZE * ((size + (DSIZE) + (WSIZE - 1)) / WSIZE);

  if (oldsize > newsize) {
    p = realloc_place(tmp, newsize, oldsize);
    newptr = chunk2mem(p);
    DPRINT("place [0x%lx] -> [0x%lx]\n", oldsize, newsize);
    return newptr;
  } else if (oldsize < newsize) {
    p = realloc_coalesced(tmp, newsize, oldsize);
    if (p == NULL) {
      newptr = mm_malloc(newsize);
      memcpy(newptr, ptr, oldsize);
      mm_free(ptr);
      DPRINT("re malloc \n");
      return newptr;
    } else {
      newptr = chunk2mem(p);
      if (newptr == ptr) {
        DPRINT("coalesced next\n");
        return newptr;
      } else {
        memcpy(newptr, ptr, oldsize - WSIZE);
        realloc_place(p, newsize, GETSIZE(p));
        DPRINT("coalesced prev\n");
        return newptr;
      }
    }
  }
  DPRINT("return ptr \n");
  return ptr;
}
static void *realloc_coalesced(void *tmp, size_t newsize, size_t oldsize) {
  void *p;

  size_t prev_use = prev_isuse(tmp);
  size_t next_use = isuse((NEXT(tmp)));
  size_t size = oldsize;

  if (prev_use && next_use) {
  } else if ((!next_use)) {
    size += GETSIZE((NEXT(tmp)));

    if (size >= newsize) {
      unlink(NEXT(tmp));

      set_inuse(tmp, size);
      return realloc_place(tmp, newsize, size);
    }
  } else if ((!prev_use) && next_use) {
    size += GETSIZE((PREV(tmp)));
    if (size >= newsize) {
      unlink(PREV(tmp));

      set_inuse(PREV(tmp), size);
      return PREV(tmp);
    }
  } else if ((!prev_use) && (!next_use)) {
    size += GETSIZE((PREV(tmp))) + GETSIZE((NEXT(tmp)));
    if (size >= newsize) {
      unlink(PREV(tmp));
      unlink(NEXT(tmp));

      set_inuse(PREV(tmp), size);
      return PREV(tmp);
    }
  }
  return NULL;
}

static void *realloc_place(void *tmp, size_t newsize, size_t oldsize) {

  // this way will be slow
  /*if ((oldsize - newsize) > (2 * DSIZE)) {*/
  /*set_inuse(tmp, newsize);*/
  /*unuse(NEXT(tmp), (oldsize - newsize));*/
  /*} else {*/
  /*set_inuse(tmp, oldsize);*/
  /*}*/

  // this way will be fast
  set_inuse(tmp, oldsize);

  return tmp;
}
