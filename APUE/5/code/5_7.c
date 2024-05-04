#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char name[L_tmpnam], line[1024];
    FILE *fp;

    char *ptr = tmpnam(NULL);
    printf("tmpnam[%p]: %s\n", ptr, ptr); /* first temp name */
    tmpnam(name);
    printf("tmpnam[%p]: %s\n", ptr, name); /* first temp name */
    if ((fp = tmpfile()) == NULL)
        /* create temp file */
        fprintf(stderr, "tmpfile error");
    fputs("one line of output\n", fp); /* write to temp file */
    rewind(fp);
    /* then read it back */

    // sleep(10);
    if (fgets(line, sizeof(line), fp) == NULL)
        fprintf(stderr, "fgets error");
    fputs(line, stdout);
    /* print the line we wrote */

    char *val = mkdtemp("/tmp/asdfasdf");
    fprintf(stdout, "%p, %s\n", val, val);

    exit(0);
}
