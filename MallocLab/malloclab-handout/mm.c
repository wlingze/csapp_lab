/*
 * Perf index = 55 (util) + 40 (thru) = 95/100
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

#define BINS_SIZE 10

#define WSIZE 8
#define DSIZE (2 * WSIZE)

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
#define get_newsize(size)                                                      \
  ((size <= 4 * WSIZE) ? (4 * WSIZE)                                           \
                       : (((size + 2 * WSIZE - 1) / WSIZE) * WSIZE))

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
  { set_unuse(p, size); }

#define FD(p) (*(size_t *)pFD(p))
#define BK(p) (*(size_t *)pBK(p))

#define pFD(p) (((void *)chunk_index(p, WSIZE)))
#define pBK(p) (((void *)chunk_index(p, 2 * WSIZE)))

#define chunk2mem(p) (chunk_index(p, WSIZE))
#define mem2chunk(mem) (chunk_index(mem, -WSIZE))

// bins
#define bin_first(p) (GET(p))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*#define DEBUG*/

#ifdef DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#define CHECK() check();
#else
#define DPRINT(...)
#define CHECK()
#endif

void *heap_list;
void *seg_bins[BINS_SIZE + 1];

static void *extend_heap(size_t word);
static void *coalesced(void *p);
static void *place(void *p, size_t size);
static void *find_fit(size_t size);
static void *realloc_coalesced(void *tmp, size_t newsize, size_t oldsize);
static void *realloc_place(void *tmp, size_t newsize, size_t oldsize);
static void bin_coalesced(size_t size);
static void mm_chunk_check();
static void mm_bins_check();
static void check();
static void mm_unlink(void *p);
static void mm_link(void *p);

static void check() {
  mm_chunk_check();
  mm_bins_check();
}

/*
 *  insert chunk into linked list.
 *      ensure that the `seg_bins` pointer always points to the smallest chunk
 *      into doubly linked list.
 *
 */
static void mm_link(void *p) {
  DPRINT("mm_link (%p)\n", p);
  void *link, *tmp, *bp;

  // get the corresponding index
  size_t index = 0;
  for (int j = GETSIZE(p); (j > 1) && (index < BINS_SIZE + 4);) {
    j >>= 1;
    index++;
  }
  index -= 4;

  // get a pointer to linked list
  link = seg_bins[index];

  if ((tmp = bp = link) == NULL) {
    // the current list is empty.

    FD(p) = (size_t)p;
    BK(p) = (size_t)p;

    seg_bins[index] = p;
    return;

  } else if (GETSIZE(link) >= GETSIZE(p)) {
    // a new minimum chunk is inserted.

    // put it in front of the original header.
    BK(p) = BK(bp);
    BK(bp) = (size_t)p;
    FD(p) = (size_t)bp;
    FD(BK(p)) = (size_t)p;

    // the pointer always points to the smallest chunk.
    seg_bins[index] = p;

    return;
  } else if (GETSIZE(BK(link)) <= GETSIZE(p)) {
    // inserted the new largest chunk.
    //      insert to the end.

    bp = (void *)BK(link);

    FD(p) = FD(bp);
    FD(bp) = (size_t)p;
    BK(p) = (size_t)bp;
    BK(FD(p)) = (size_t)p;

    return;
  } else {
    // insert in the middle.

    // traverse from the smallest
    // to get the first chunk larger than p.
    do {
      if (GETSIZE(tmp) >= GETSIZE(p)) {
        bp = tmp;
        break;
      }
      tmp = (void *)FD(tmp);
    } while (tmp != link);

    // insert `p` into the BK of this chunk.
    BK(p) = BK(bp);
    BK(bp) = (size_t)p;
    FD(p) = (size_t)bp;
    FD(BK(p)) = (size_t)p;
  }
}

/*
 *  remove chunk from linked list.
 *      if the linked list is empty, the `seg_bins` pointer must be clear.
 */
static void mm_unlink(void *p) {
  DPRINT("mm_unlink(%p)\n", p);
  void *link, *tmp;

  // get the corresponding index.
  size_t index = 0;
  for (int j = GETSIZE(p); (j > 1) && (index < BINS_SIZE + 4);) {
    j >>= 1;
    index++;
  }
  index -= 4;

  link = seg_bins[index];

  if ((FD(p) == (size_t)p) && (BK(p) == (size_t)p) && (link == p)) {
    // if there is only one chunk,
    // empty after removal.
    seg_bins[index] = 0;
    FD(p) = 0;
    BK(p) = 0;
    return;
  }

  if (link == p) {
    // if the pointer points to p,
    // move to the FD(p).
    seg_bins[index] = (void *)FD(link);
  }

  // remove p.
  BK(FD(p)) = BK(p);
  FD(BK(p)) = FD(p);
  FD(p) = 0;
  BK(p) = 0;

  // CHECK();
  return;
}

static void mm_chunk_check() {
  void *tmp = heap_list;
  DPRINT("|||| chunk\n");
  while (GETSIZE(tmp) != 0) {
    DPRINT("   |\\-> [%p](%lx)(0x%lx)\n", tmp, isuse(tmp), GETSIZE(tmp));
    tmp = (void *)NEXT(tmp);
  }
  DPRINT("   |\\-> [%p](%lx)(0x%lx)\n", tmp, isuse(tmp), GETSIZE(tmp));
  return;
}

static void mm_bins_check() {
  void *link, *tmp;
  size_t index = 0;
  DPRINT("|||| bin \n");
  while (index <= BINS_SIZE) {
    DPRINT("  || bin[%ld] \n", index);
    tmp = link = seg_bins[index];
    while (tmp != 0) {
      DPRINT("   |\\-> [%p](0x%lx)\n", tmp, GETSIZE(tmp));
      tmp = (void *)FD(tmp);
      if (tmp == link) {
        break;
      }
    }
    index++;
  }
  return;
}

/*
 * preform free heap expansion
 * the default is 0x1000
 *
 *      is called:
 *          mm_init: initialize a large heap for allocation.
 *          mm_malloc: the existing free heap block connot satisfy the
 *                     allocation, allocate a large pile of block.
 */
static void *extend_heap(size_t word) {
  DPRINT("extend_heap (%lx)\n", word);

  void *bp;
  size_t size = (word % 2) ? ((word + 1) * WSIZE) : (word * WSIZE);

  if ((bp = mem_sbrk(size)) == (void *)-1)
    return NULL;

  unuse(bp, size);
  PUT((NEXT(bp)), (CHUNK_INUSE));

  return coalesced(bp);
}

/*
 * try to merge
 */
static void *coalesced(void *p) {
  DPRINT("coalesced(%p)\n", p);

  size_t prev_use = prev_isuse(p);
  size_t next_use = isuse((NEXT(p)));
  size_t size = GETSIZE((p));

  if (prev_use && next_use) {
    // unable to merge
  } else if (prev_use && (!next_use)) {
    // merge backwards
    mm_unlink(NEXT(p));
    size += GETSIZE((NEXT(p)));
  } else if ((!prev_use) && next_use) {
    // merge forward
    mm_unlink(PREV(p));
    size += GETSIZE((PREV(p)));
    p = PREV(p);
  } else if ((!prev_use) && (!next_use)) {
    // merge forward and backward at the same time
    mm_unlink(PREV(p));
    mm_unlink(NEXT(p));
    size += GETSIZE((PREV(p))) + GETSIZE((NEXT(p)));
    p = PREV(p);
  }

  // at the same time
  // `mm_link` work is also carried out
  unuse(p, size);
  mm_link(p);

  return p;
}

/*
 * cut chunk.
 *
 */
static void *place(void *p, size_t size) {
  DPRINT("place(%p, %lx)\n", p, size);
  size_t psize = GETSIZE((p));
  if ((psize - size) < (2 * DSIZE)) {
    // if the difference is less than the minimum chunk size,
    // do not cut.
    inuse(p, psize);
  }

  // optimization of `./traces/binary*`
  else if (size >= 88) {
    // if it is a lareg chunk,
    // it will be partially returned after being cut out.
    unuse(p, (psize - size));
    mm_link(p);
    inuse(NEXT(p), size);
    return NEXT(p);
  } else {
    // if it is a small chunk,
    // cut out the previous part and return.
    inuse(p, size);
    unuse(NEXT(p), (psize - size));
    mm_link(NEXT(p));
  }
  return p;
}

/*
 * find whether the free chunk meets the requirements
 */
static void *find_fit(size_t size) {
  DPRINT("find_fit(0x%lx)\n", size);
  void *link, *tmp;

  // get the corresponding index
  size_t index = 0;
  for (int j = size; (j > 1) && (index < BINS_SIZE + 4);) {
    j >>= 1;
    index++;
  }
  index -= 4;

  // make a search
  while (1) {
    link = seg_bins[index];
    //  linked list is not empty
    if (link != NULL) {
      // if the largest chunk cannot be satisfied,
      // the next index.
      if (GETSIZE(BK(link)) >= size) {
        tmp = link;
        do {
          if (size <= GETSIZE(tmp)) {
            DPRINT("find [%p](0x%lx)\n", tmp, GETSIZE(tmp));
            return tmp;
          }
          tmp = (void *)FD(tmp);
        } while (tmp != link);
      }
    }

    index++;
    if (index > BINS_SIZE) {
      return NULL;
    }
  }
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  DPRINT("\nmm init!\n");
  if ((heap_list = mem_sbrk((4) * WSIZE)) == (void *)-1)
    return -1;
  set_inuse(heap_list, (4) * WSIZE);
  PUT(NEXT(heap_list), (CHUNK_INUSE | PREV_INUSE));

  for (int i = 0; i <= BINS_SIZE; i++) {
    seg_bins[i] = 0;
  }

  if (extend_heap(CHUNK_SIZE / WSIZE) == NULL)
    return -1;
  CHECK();
  return 0;
}

/* malloc function
 * heap allocation
 */
void *mm_malloc(size_t size) {
  DPRINT("mm malloc(0x%lx)\n", size);
  int newsize;
  void *tmp;

  // the heap_list is null,
  // indicating that it is not initialize
  if (heap_list == NULL) {
    mm_init();
  }

  /*invalid parameter*/
  if (size == 0) {
    return NULL;
  }

  /*memory size => chunk size*/
  newsize = get_newsize(size);

  /*
   * try to find a suitable one in the free heap block
   */
  if ((tmp = find_fit(newsize)) != NULL) {
    mm_unlink(tmp);
    tmp = place(tmp, newsize);
    DPRINT("malloc(0x%x) [%p] <- bin\n", newsize, tmp);
    CHECK();
    return chunk2mem(tmp);
  }

  /*perform free heap expansion */
  size_t extend_size = MAX(newsize, CHUNK_SIZE);
  if ((tmp = extend_heap(extend_size / WSIZE)) == NULL) {
    DPRINT("malloc(0x%x) [%p] failure\n", newsize, tmp);
    CHECK();
    return NULL;
  }

  mm_unlink(tmp);
  tmp = place(tmp, newsize);

  DPRINT("malloc(0x%x) [%p] <- extend\n", newsize, tmp);
  CHECK();
  return chunk2mem(tmp);
}

void mm_free(void *ptr) {
  DPRINT("mm_free(%p)\n", ptr);

  /*invalid parameter*/
  if (ptr == 0)
    return;

  /*get the corresponding pointer and size */
  void *tmp = mem2chunk(ptr);
  size_t size = GETSIZE(tmp);

  unuse(tmp, size);
  coalesced(tmp);

  DPRINT("fastbin free[%p](0x%lx)\n", tmp, size);
  CHECK();
}

void *mm_realloc(void *ptr, size_t size) {

  DPRINT("\nrealloc [%p](0x%lx) -> (0x%lx)\n", mem2chunk(ptr),
         GETSIZE(mem2chunk(ptr)), size);

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
      CHECK();
      return newptr;
    } else {
      newptr = chunk2mem(p);
      if (newptr == ptr) {
        DPRINT("coalesced next\n");
        CHECK();
        return newptr;
      } else {
        memmove(newptr, ptr, oldsize - WSIZE);
        realloc_place(p, newsize, GETSIZE(p));
        DPRINT("coalesced prev\n");
        CHECK();
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
      mm_unlink(NEXT(tmp));

      set_inuse(tmp, size);
      return realloc_place(tmp, newsize, size);
    }
  } else if ((!prev_use) && next_use) {
    size += GETSIZE((PREV(tmp)));
    if (size >= newsize) {
      mm_unlink(PREV(tmp));

      set_inuse(PREV(tmp), size);
      return PREV(tmp);
    }
  } else if ((!prev_use) && (!next_use)) {
    size += GETSIZE((PREV(tmp))) + GETSIZE((NEXT(tmp)));
    if (size >= newsize) {
      mm_unlink(PREV(tmp));
      mm_unlink(NEXT(tmp));

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
  /*mm_link(NEXT(tmp));*/
  /*} else {*/
  /*set_inuse(tmp, oldsize);*/
  /*}*/

  // this way will be fast
  set_inuse(tmp, oldsize);

  return tmp;
}
