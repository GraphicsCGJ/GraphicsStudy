#include <unistd.h>
#include <sys/wait.h>

char *env_init[] = { "USER=unknown", "PATH=/tmp", '\0' };

int main(void)
{
    pid_t pid;

    // First fork and execle
    if ((pid = fork()) < 0) {
        perror("fork error");
    } else if (pid == 0) { /* specify pathname, specify environment */
        if (execle("/home/sar/bin/echoall", "echoall", "myarg1", "MY ARG2", (char *)0, env_init) < 0)
            perror("execle error");
    }

    // Wait for the first child process
    if (waitpid(pid, '\0', 0) < 0)
        perror("wait error");

    // Second fork and execlp
    if ((pid = fork()) < 0) {
        perror("fork error");
    } else if (pid == 0) { /* specify filename, inherit environment */
        if (execlp("echoall", "echoall", "only 1 arg", (char *)0) < 0)
            perror("execlp error");
    }

    exit(0);
}
