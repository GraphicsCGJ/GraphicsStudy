#include <stdio.h>
#include <stdlib.h>

int main(void) {

  char *null_str = NULL;
  char *null_str2 = NULL;

  memcpy(null_str, null_str2, 10);

  return 0;
}