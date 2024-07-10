#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        perror("fork error");
    else if (pid != 0) {
        sleep(2);
        exit(2); /* terminate with exit status 2 */
    }

    if ((pid = fork()) < 0)
        perror("fork error");
    else if (pid != 0) {
        sleep(4);
        abort(); /* first child: terminate with core dump */
    }

    if ((pid = fork()) < 0)
        perror("fork error");
    else if (pid != 0) {
        /* second child */
        execl("/bin/dd", "dd", "if=/etc/passwd", "of=/dev/null", NULL);
        exit(7); /* shouldn’t get here */
    }

    if ((pid = fork()) < 0)
        perror("fork error");
    else if (pid != 0) {
        sleep(8);
        exit(0); /* third child: normal exit */
    }

    sleep(6);
    kill(getpid(), SIGKILL);
    exit(6); /* fourth child: terminate w/signal, no core dump */
    /* shouldn’t get here */
}
