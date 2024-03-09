#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {

  int fd = open("out", O_WRONLY | O_CREAT | O_TRUNC);
  write(fd, "asdfasdf", 8);
  close(fd);

  return 0;
}