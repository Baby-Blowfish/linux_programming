#include <stdio.h>              // printf
#include <sys/wait.h>           // waitpid
#include <spawn.h>              // posix_spawn 관련
#include <unistd.h>             // pid_t

// 외부 환경 변수 참조 (exec에서 환경 유지 목적)
extern char **environ;

// 사용자 정의 system() 함수
int system(char *cmd)
{
    pid_t pid;
    int status;

    // 파일 디스크립터 관련 동작 설정 구조체
    posix_spawn_file_actions_t actions;

    // 프로세스 속성 설정 구조체
    posix_spawnattr_t attrs;

    // 실행할 셸 명령어: /bin/sh -c "cmd"
    char *argv[] = {"sh", "-c", cmd, NULL};

    // 파일 액션 구조체 초기화 (필요시 입출력 리디렉션 등 추가 가능)
    posix_spawn_file_actions_init(&actions);

    // 속성 구조체 초기화
    posix_spawnattr_init(&attrs);

    // 스케줄러 설정 가능하도록 설정 플래그 추가 (옵션, 없어도 됨)
    posix_spawnattr_setflags(&attrs, POSIX_SPAWN_SETSCHEDULER);

    // posix_spawn으로 자식 프로세스 생성
    // → 이 시점에서 자식은 "/bin/sh -c cmd" 실행
    posix_spawn(&pid, "/bin/sh", &actions, &attrs, argv, environ);

    // 자식 프로세스가 종료될 때까지 기다림
    waitpid(pid, &status, 0);

    return status; // 자식 종료 상태 반환
}


int main(int argc, char **argv, char **envp)
{
    // 현재 환경 변수 출력
    while (*envp)
        printf("%s\n", *envp++);

    // 사용자 정의 system() 함수로 명령어 실행
    system("who");         // 로그인 사용자 출력
    system("nocommand");   // 없는 명령 → 오류 메시지 출력
    system("cal");         // 달력 출력

    return 0;
}

