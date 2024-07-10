#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

int system(const char *cmdstring)
{
    pid_t pid;
    int status;

    /* Version without signal handling */

    if (cmdstring == NULL)
        return 1; /* Return nonzero if command string is NULL */

    /* Always a command processor with UNIX */
    if ((pid = fork()) < 0) {
        status = -1; /* Fork error */
    } else if (pid == 0) {
        /* Child process */
        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        _exit(127); /* Exec error */
    } else {
        /* Parent process */
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1; /* Error other than EINTR from waitpid() */
                break;
            }
        }
    }

    return status;
}

void pr_exit(int status) {
    if (WIFEXITED(status)) {
        printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("abnormal termination, signal number = %d%s\n",
               WTERMSIG(status),
#ifdef WCOREDUMP
               WCOREDUMP(status) ? " (core file generated)" : "");
#else
               "");
#endif
    } else if (WIFSTOPPED(status)) {
        printf("child stopped, signal number = %d\n", WSTOPSIG(status));
    }
}

int main(void)
{
    int status;
    printf("Case 1\n\n");
    if ((status = system("date")) < 0)
        perror("system() error");
    pr_exit(status);

    printf("Case 2\n\n");
    if ((status = system("nosuchcommand")) < 0)
        perror("system() error");
    pr_exit(status);

    printf("Case 3\n\n");
    if ((status = system("who; exit 44")) < 0)
        perror("system() error");
    pr_exit(status);

    exit(0);
}
