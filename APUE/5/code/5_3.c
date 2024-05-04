#include <stdio.h>

int main(void) {
    // char c = getc(stdin);
    // printf("%c\n", c);

    printf("address of getc:\t%p\n", getc);
    printf("address of fgetc:\t%p\n", fgetc);

    FILE *fp = fopen("5_3.c", "r'");
    printf("%c\n", fgetc(fp));
    return 0;
}