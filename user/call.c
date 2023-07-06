#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  unsigned int i = 0x00646c72;
  // printf("i = %d\n", i);
  // printf("i = %d\n", &i);
  printf("H%x Wo%s\n", 57616, &i);

  printf("x=%d y=%d\n", 3);

  printf("%d %d\n", f(8)+1, 13);
  exit(0);
}
