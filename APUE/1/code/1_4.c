
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  struct stat st;

  if (stat(argv[1], &st) == -1) {
    perror("stat");
    return 1;
  }

  printf("st.st_size: %d\n", st.st_size);

  return 0;
}