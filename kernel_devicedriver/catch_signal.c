#include <stdio.h>      // 표준 입출력 라이브러리
#include <stdlib.h>     // 일반 유틸리티 함수들 (예: exit)
#include <string.h>     // 문자열 처리 함수들 (예: memset, strlen)
#include <signal.h>     // 시그널 처리 함수들 (예: signal)
#include <fcntl.h>      // 파일 제어 관련 함수들 (예: open)
#include <unistd.h>     // 유닉스 표준 함수들 (예: read, write)

// 시그널 핸들러 함수 정의
void signal_handler (int signum)
{
    // 시그널이 감지되었음을 출력
	printf("Signal is Catched!!!\n");

    // SIGIO 시그널이 들어왔을 때의 처리
	if(signum == SIGIO)
	{
		printf("SIGIO\r\n");
		exit(1);  // 프로그램 종료
	}
}

int main(int argc, char **argv)
{
	char buf[BUFSIZ];  // 데이터를 저장할 버퍼
	char i = 0;
	int fd = -1;       // 파일 디스크립터 초기화
	memset(buf, 0, BUFSIZ);  // 버퍼를 0으로 초기화

	signal(SIGIO, signal_handler);  // SIGIO 시그널에 대해 핸들러 등록

	// 입력 인자 개수가 2개가 아니면 사용법 출력
	if(argc != 2)
	{
		printf("Usage : %s Command", argv[0]);
		return 0;  // 프로그램 종료
	}

	// 입력된 GPIO 명령어 출력
	printf("GPIO Set : %s\n", argv[1]);

	// "/dev/gpiosignal" 장치 파일을 읽기/쓰기 모드로 염
	fd = open("/dev/gpiosignal", O_RDWR);

	// 버퍼에 명령어와 프로세스 ID를 저장
	sprintf(buf, "%s:%d", argv[1], getpid());

	// 장치에 명령어와 PID를 씀
	write(fd, buf, strlen(buf));

	// 장치로부터 데이터를 읽고 성공 시 메시지 출력
	if(read(fd, buf, strlen(buf)) != 0)
		printf("Success : read()\n");

	// 읽어온 데이터 출력
	printf("Read Data : %s\n", buf);

	// 현재 프로세스 ID 출력
	printf("My PID is %d.\n", getpid());

	// 무한 루프 (시그널을 받을 때까지 대기)
	while(1);

	// 파일 디스크립터 닫기
	close(fd);

	return 0;
}
