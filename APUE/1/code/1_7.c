#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {

  int fd = open("xxxxx", O_RDONLY);
  printf("fd:%d\n", fd);
  printf("errno: %d[%s]\n", errno, strerror(errno));
  close(fd);

  return 0;
}