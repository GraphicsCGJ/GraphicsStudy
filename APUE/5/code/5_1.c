#include <stdio.h>
#include <wchar.h>

int main(void) {
    FILE *fp;

    fp = fopen("asdfasdf", "w");
    fprintf(fp, "hihihi\n");
    printf("fwide: %d\n", fwide(fp, 0));
    fclose(fp);

    return 0;
}