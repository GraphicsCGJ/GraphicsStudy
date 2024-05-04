#include <stdio.h>

int main(void) {
    int width = 3;
    int value = 5;
    float fvalue = 1234.3413241f;
    printf("%*d, width, value\n", width, value);
    printf("%*.*f, width, value\n", width, width, fvalue);
    return 0;
}