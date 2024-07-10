#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

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

int main(int argc, char *argv[]) {
    int status;

    if (argc < 2)
        fprintf(STDERR_FILENO, "command-line argument required");

    if ((status = system(argv[1])) < 0)
        perror("system() error");

    pr_exit(status);
    exit(0);
}
