/**
 * 소스코드를 옛날로 돌려야 동작한다.
 * 터미널에 getconf _POSIX_VERSION 라고 검색 시 2008 버전 이상의 C 버전이 사용되고 있을텐데
 * 소스코드를 과거로 돌려야 fdopen이 보인다. deprecated된듯?
 *
 * 참고 링크는 하단과 같다.
 * https://stackoverflow.com/questions/15944051/error-fdopen-was-not-declared-found-with-g-4-that-compiled-with-g3
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

int pipes[2];

#define PIPE_READ pipes[0]
#define PIPE_WRITE pipes[1]

#define PARENT_DEFAULT 0
#define PARENT_FDOPEN 1

static void child_job() {
    char buf[1024];
    int i;

    for (i = 0; i < 20; i++) {
        // sprintf(buf, "DATA[%d]", i);
        sprintf(buf, "DATA[%d]\n", i);
        write(PIPE_WRITE, buf, strlen(buf));
        usleep(100);
    }

    close(PIPE_WRITE);
}

static void parent_job(int input) {
    char buf[1024];
    int times = 0;
    switch (input) {
    case PARENT_FDOPEN: {
        FILE* fp = fdopen(PIPE_READ, "r");
        while (1) {
            int res = fscanf(fp, "%s", buf);
            if (res < 0) {
                printf("recved error, %s\n", strerror(errno));
                break;
            }
            else if (res == 0) {
                printf("recved disconnected, %s\n", strerror(errno));
                break;
            }

            /* buf gained by fscanf, auto null termination. */
            printf("recved data[%d]: [%s]\n", times++, buf);
        }
        close(PIPE_READ);
        break;
    }
    default: {
        while (1) {
            int res = read(PIPE_READ, buf, sizeof(buf));
            if (res < 0) {
                printf("recved error, %s\n", strerror(errno));
                break;
            }
            else if (res == 0) {
                printf("recved disconnected, %s\n", strerror(errno));
                break;
            }
            buf[res] = '\0';
            printf("recved data[%d]: %s\n", times++, buf);
        }
        close(PIPE_READ);
        break;
    }
    }

}

int main(void) {
    /* 0 - stdin
       1 - stdout
       2 - stderr
       3 - readpipe
       4 - writepipe
    */
    pipe(pipes);

    int pid = fork();

    if (pid == 0) { /* child */
        close(PIPE_READ);
        child_job();
    } else if (pid > 0) { /* parent */
        close(PIPE_WRITE);
        // parent_job(PARENT_DEFAULT);
        parent_job(PARENT_FDOPEN);
        waitpid(pid, NULL, 0);
    }

    return 0;
}