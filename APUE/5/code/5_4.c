#include <stdio.h>

int main(void) {
    FILE *fp = fopen("5_4_out", "w");
    fputs("String1", fp);
    fputs("String2", fp);
    fputs("String3", fp);

    puts("String1");
    puts("String2");
    puts("String3");


    fclose(fp);
    return 0;
}