#define NOBITA_LINUX_IMPL
#include "nobita-linux.h"

#include <stdio.h>

struct c {
  int *nums;
  size_t nums_used;
  size_t nums_size;
};

void build(Nobita_Build *b) {
  (void)b;

  struct c aa;
  vector_init(&aa, nums);

  for (int i = 0; i < 1024; i++)
    vector_append(&aa, nums, i);

  for (int i = 0; i < 1024; i++)
    vector_append(&aa, nums, i * 2);

  for (size_t i = 0; i < aa.nums_used; i++)
    printf("%d\n", aa.nums[i]);

  vector_free(&aa, nums);
}
