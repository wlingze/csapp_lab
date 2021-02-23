#include <stdio.h>
#include <stdlib.h>

#define WSIZE 8
#define bins_index(size)                                                       \
  {                                                                            \
    int __index = 0;                                                           \
    size_t __size = size;                                                      \
    while (__size > 10) {                                                      \
      __index += 1;                                                            \
      __size >>= 1;                                                            \
      if (__index > 7) {                                                       \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    __index = (__index - 1) * WSIZE;                                           \
  }

int main() {
  void *tmp;
  tmp = (void *)malloc(0x30);
  *((int *)tmp) = 9;
  size_t a;

  a = (bins_index(8096));
  printf("arst%ld", a);
  return 0;
}
