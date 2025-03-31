#include<stdio.h>
#include<unistd.h>

int main(int argc, char **argv)
{
	int i;

	for(i=0;;i++)
	{
		printf("%10d\r",i);
		fflush(NULL);
		sleep(1);
	}

	return 0;
}



/*
 * ./loop : 포어그라운드에서 무한 루프로 실햏ㅇ -> 종료하려면 ctl+c
 * ./loop & : 백그라운드 실행, 현재의 작업 번호가 표시됨
 * fg *jop번호 : 해당 job번호 프로세스를 포그라운드 모드로 바꿈
 * jobs : 현재의 작업들을 볼수있음
