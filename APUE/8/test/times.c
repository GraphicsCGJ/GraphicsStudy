#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>

static void pr_times(clock_t, struct tms *, struct tms *);
static void do_cmd(char *);

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
    int i;

    setbuf(stdout, NULL); // 표준 출력 버퍼링을 비활성화

    // 각 명령줄 인자마다 do_cmd 함수 실행
    for (i = 1; i < argc; i++)
        do_cmd(argv[i]);

    exit(0);
}

static void do_cmd(char *cmd) {
    struct tms tmsstart, tmsend;
    clock_t start, end;
    int status;

    printf("\ncommand: %s\n", cmd);

    // 시작 시간 측정
    if ((start = times(&tmsstart)) == -1)
        perror("times error");

    // 명령 실행
    if ((status = system(cmd)) < 0)
        perror("system() error");

    // 종료 시간 측정
    if ((end = times(&tmsend)) == -1)
        perror("times error");

    // 실행 시간 및 CPU 사용량 출력
    pr_times(end - start, &tmsstart, &tmsend);
    pr_exit(status);
}

static void pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend) {
    static long clktck = 0;

    // 클럭 틱 값 가져오기
    if (clktck == 0) {
        if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
            perror("sysconf error");
    }

    // 실제 경과 시간 출력
    printf(" real: %7.2f\n", real / (double) clktck);

    // 사용자 CPU 시간 출력
    printf(" user: %7.2f\n",
           (tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck);

    // 시스템 CPU 시간 출력
    printf(" sys:  %7.2f\n",
           (tmsend->tms_stime - tmsstart->tms_stime) / (double) clktck);

    // 자식 프로세스의 사용자 CPU 시간 출력
    printf(" child user: %7.2f\n",
           (tmsend->tms_cutime - tmsstart->tms_cutime) / (double) clktck);

    // 자식 프로세스의 시스템 CPU 시간 출력
    printf(" child sys:  %7.2f\n",
           (tmsend->tms_cstime - tmsstart->tms_cstime) / (double) clktck);
}
