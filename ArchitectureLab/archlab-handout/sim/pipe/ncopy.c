#include <stdio.h>

typedef word_t word_t;

word_t src[8], dst[8];

/* $begin ncopy */
/*
 * ncopy - copy src to dst, returning number of positive ints
 * contained in src array.
 */
word_t ncopy(word_t *src, word_t *dst, word_t len) {
  word_t count = 0;
  word_t val;

  while (len > 0) {
    val = *src++;
    *dst++ = val;
    if (val > 0)
      count++;
    len--;
  }
  return count;
}
/* $end ncopy */

word_t ncopy2(word_t *src, word_t *dst, word_t len) {
  word_t count = 0;
  word_t val;
  word_t val0, val1, val2, val3;

  while (len - 4 > 0) {
    val0 = *(src + 0);
    val2 = *(src + 1);
    val3 = *(src + 2);
    val3 = *(src + 3);

    *(dst + 0) = val0;
    *(dst + 1) = val2;
    *(dst + 2) = val2;
    *(dst + 3) = val3;

    src += 4;
    dst += 4;
    len -= 4;

    if (val0 > 0)
      count++;
    if (val1 > 0)
      count++;
    if (val2 > 0)
      count++;
    if (val3 > 0)
      count++;
  }

  while (len > 0) {
    val = *src++;
    *dst++ = val;
    if (val > 0)
      count++;
    len--;
  }

  return count;
}

word_t ncopy3(word_t *src, word_t *dst, word_t len) {
  int count = 0;

  while (len - 8) {
  }

  while (len - 4) {
  }

  while (len) {
  }

  return count;
}

int main() {
  word_t i, count;

  for (i = 0; i < 8; i++)
    src[i] = i + 1;
  count = ncopy(src, dst, 8);
  printf("count=%d\n", count);
  exit(0);
}
