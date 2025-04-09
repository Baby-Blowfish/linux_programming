/*
 * 데몬 프로세스 구현 예제
 * 데몬 프로세스는 백그라운드에서 실행되며, 터미널과 독립적으로 동작하는 프로세스입니다.
 * 주요 특징:
 * 1. 부모 프로세스와 분리
 * 2. 터미널 제어권 해제
 * 3. 작업 디렉토리 변경
 * 4. 파일 권한 설정
 * 5. 표준 입출력 재지정
 */

#include <stdio.h>      // 표준 입출력 함수
#include <fcntl.h>      // 파일 제어 함수
#include <signal.h>     // 시그널 처리
#include <string.h>     // 문자열 처리
#include <syslog.h>     // 시스템 로깅
#include <unistd.h>     // UNIX 표준 함수
#include <sys/stat.h>   // 파일 상태
#include <sys/resource.h> // 리소스 제한

int main(int argc, char **argv)
{
    struct sigaction sa;    // 시그널 핸들러 설정을 위한 구조체
    struct rlimit rl;       // 리소스 제한을 위한 구조체
    int fd0, fd1, fd2, i;  // 파일 디스크립터
    pid_t pid;             // 프로세스 ID

    // 명령행 인자 검사
    if(argc < 2)
    {
        printf("Usage : %s command\n",argv[0]);
        return -1;
    }
    
    /* 파일 생성 시 기본 권한 설정
     * umask(0)는 모든 권한을 허용하도록 설정
     * 기본 umask는 022로, 그룹과 기타 사용자의 쓰기 권한을 제한
     */
    umask(0);

    /* 파일 디스크립터 제한 확인
     * RLIMIT_NOFILE: 프로세스가 열 수 있는 최대 파일 디스크립터 수
     * 이 값은 시스템의 리소스 제한을 확인하는데 사용됨
     */
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        perror("getrlimit");
    }

    /* 첫 번째 fork() 호출
     * 부모 프로세스와 자식 프로세스를 분리
     * 자식 프로세스는 새로운 세션의 리더가 될 수 있음
     */
    if((pid=fork())<0)
    {
        perror("fork()");
        return -1;
    }
    else if(pid != 0)       // 부모 프로세스 종료
    {
        return 0;
    }

    /* setsid() 호출
     * 새로운 세션을 생성하고 프로세스 그룹 리더가 됨
     * 터미널과의 연결을 끊고 독립적인 세션을 만듦
     */
    setsid();

    /* SIGHUP 시그널 무시 설정
     * 터미널이 닫힐 때 발생하는 SIGHUP 시그널을 무시
     * 데몬이 터미널 종료에 영향을 받지 않도록 함
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL)< 0)
    {
        perror("sigaction() : Can't ignore SIGHUP");
    }

    /* 작업 디렉토리를 루트(/)로 변경
     * 데몬이 실행 중인 파일 시스템이 언마운트되는 것을 방지
     */
    if(chdir("/")<0)
    {
        perror("cd()");
    }

    /* 파일 디스크립터 제한 설정
     * 무한대인 경우 1024로 제한
     */
    if(rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }

    /* 모든 파일 디스크립터 닫기
     * 상속받은 파일 디스크립터를 모두 닫아 리소스 낭비 방지
     */
    for(i=0; i < rl.rlim_max; i++)
    {
        close(i);
    }

    /* 표준 입출력 재지정
     * /dev/null로 표준 입력, 출력, 에러를 리다이렉트
     * 데몬이 터미널 입출력을 사용하지 않도록 함
     */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    /* 시스템 로그 초기화
     * 데몬의 로그를 시스템 로그에 기록
     */
    openlog(argv[1], LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    /* 데몬 시작 로그 기록 */
    syslog(LOG_INFO, "Daemon Process");
    
    /* 데몬의 메인 루프
     * 실제 데몬의 작업이 여기에 구현됨
     */
    while(1){}

    /* 시스템 로그 종료 */
    closelog();
    
    return 0;

    /* 시스템 로그 확인을 위한 설정
     * Ubuntu/Debian: sudo apt install rsyslog
     * 로그 확인: tail -f /var/log/syslog
     */
}
