#include <unistd.h>

int globvar = 6;
int main(void)
{
    int var;
    pid_t pid;
    /* external variable in initialized data */
    /* automatic variable on the stack */
    var = 88;
    printf("before vfork\n");
    if ((pid = vfork()) < 0) {
        perror("vfork error");
    } else if (pid == 0) {
        globvar++;
        var++;
        // _exit(0);
        exit(0);
    }
    /* we don’t flush stdio */
    /* child */
    /* modify parent’s variables */
    /* child terminates */
    /* parent continues here */
    printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar,
            var);
    exit(0);
}
