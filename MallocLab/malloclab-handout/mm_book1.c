#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MINBLOCKSIZE 16

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)                                                      \
  ((size) | (alloc)) /* Pack a size and allocated bit into a word */

#define GET(p) (*(unsigned int *)(p)) /* read a word at address p */
#define PUT(p, val)                                                            \
  (*(unsigned int *)(p) = (val)) /* write a word at address p */

#define GET_SIZE(p) (GET(p) & ~0x7) /* read the size field from address p */
#define GET_ALLOC(p) (GET(p) & 0x1) /* read the alloc field from address p */

#define HDRP(bp)                                                               \
  ((char *)(bp)-WSIZE) /* given block ptr bp, compute address of its header */
#define FTRP(bp)                                                               \
  ((char *)(bp) + GET_SIZE(HDRP(bp)) -                                         \
   DSIZE) /* given block ptr bp, compute address of its footer */

#define NEXT_BLKP(bp)                                                          \
  ((char *)(bp) +                                                              \
   GET_SIZE(                                                                   \
       HDRP(bp))) /* given block ptr bp, compute address of next blocks */
#define PREV_BLKP(bp)                                                          \
  ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE)) /* given block ptr bp, compute   \
                                                 address of prev blocks */

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

static char *heap_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);

/*
 * extend heap by words * word(4 bytes)
 */


static void *extend_heap(size_t word) {

  void *bp;
  size_t size = (word % 2) ? ((word + 1) * WSIZE) : (word * WSIZE);
  // dprint("\textend_heap 0x%lx\n", size);

  if ((bp = mem_sbrk(size)) == (void *)-1)
    return NULL;

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  return coalesce(bp);
}

/*
 * coalesce
 */

static void *coalesce(void *p) {

  size_t prev_use = GET_ALLOC(FTRP(PREV_BLKP(p)));
  size_t next_use = GET_ALLOC(HDRP(NEXT_BLKP(p)));
  size_t size = GET_SIZE(HDRP(p));

  if (prev_use && next_use) {
    return p;
  } else if (prev_use & !next_use) {
    size += GET_SIZE(HDRP(NEXT_BLKP(p)));
    PUT(HDRP(p), PACK(size, 0));
    PUT(FTRP(p), PACK(size, 0));
  } else if ((!prev_use) && next_use) {
    size += GET_SIZE(HDRP(PREV_BLKP(p)));
    PUT(FTRP(p), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(p)), PACK(size, 0));
    p = PREV_BLKP(p);
  } else if ((!prev_use) && (!next_use)) {
    size += GET_SIZE(HDRP(PREV_BLKP(p))) + GET_SIZE(HDRP(NEXT_BLKP(p)));
    PUT(HDRP(PREV_BLKP(p)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));
    p = PREV_BLKP(p);
  }
  return p;
}

/*
 * find_fit - use first fit strategy to find an empty block.
 */
static void *find_fit(size_t size) {
  void *tmp = heap_listp;

  for (tmp = heap_listp; GET_SIZE(HDRP(tmp)) > 0; tmp = NEXT_BLKP(tmp)) {

    if (((!GET_ALLOC(HDRP(tmp)))) &&
        (GET_SIZE(HDRP(tmp)) >= (unsigned int)size)) {

      return tmp;
    }
  }
  return NULL;
}



/*
 * place - Place the request block at the beginning of the free block,
 *         and only split if the remaining part is equal to or larger than the
 * size of the smallest block
 */
static void place(void *p, size_t size) {

  size_t psize = GET_SIZE(HDRP(p));

  if ((psize - size) >= (2 * DSIZE)) {
    PUT(HDRP(p), PACK(size, 1));
    PUT(FTRP(p), PACK(size, 1));
    void *tmp = NEXT_BLKP(p);
    PUT(HDRP(tmp), PACK((psize - size), 0));
    PUT(FTRP(tmp), PACK((psize - size), 0));
  } else {
    PUT(HDRP(p), PACK(psize, 1));
    PUT(FTRP(p), PACK(psize, 1));
  }
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {


  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    return -1;

  PUT(heap_listp, 0);
  PUT(heap_listp + 1 * WSIZE, PACK(DSIZE, 1));
  PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));
  PUT(heap_listp + 3 * WSIZE, PACK(0, 1));
  heap_listp += 2 * WSIZE;

  if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    return -1;

  return 0;
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  int newsize;
  void *tmp;

  if (heap_listp == NULL) {
    mm_init();
  }

  if (size == 0) {
    return NULL;
  }

  if (size <= DSIZE)
    newsize = 2 * DSIZE;
  else
    newsize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);



  if ((tmp = find_fit(newsize)) != NULL) {
    place(tmp, newsize);
    return tmp;
  }

  size_t extend_size = MAX(newsize, CHUNKSIZE);
  if ((tmp = extend_heap(extend_size / WSIZE)) == NULL)

    return NULL;

  place(tmp, newsize);
  return tmp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
// void mm_free(void *bp) {
//   size_t size = GET_SIZE(HDRP(bp));

//   PUT(HDRP(bp), PACK(size, 0));
//   PUT(FTRP(bp), PACK(size, 0));
//   coalesce(bp);
// }

void mm_free(void *ptr) {

  if (ptr == 0)
    return;

  size_t size = GET_SIZE(HDRP(ptr));


  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
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

  copy_size = GET_SIZE(HDRP(ptr));

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
