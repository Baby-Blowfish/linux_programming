#include <stdio.h> 		  /* printf( ) 함수를 위해 사용 */
#include <stdlib.h> 		/* exit( ) 함수를 위해 사용 */
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h> 		/* waitpid() 함수를 위해 사용 */
#include <semaphore.h> 		/* sem_open(), sem_destroy(), sem_wait() 등 함수를 위한 헤더 파일 */
#include <fcntl.h>


static sem_t *sem; 			/* 세마포어를 위한 전역 변수 */

static void p() 		/* 세마포어의 P 연산 */
{    
  sem_wait(sem);
  printf("p\n") ;
}

static void v() 		/* 세마포어의 V 연산 */
{
  sem_post(sem);
  printf("v\n") ;
}

int main(int argc, char **argv)
{
  pid_t pid;
  int status;  
  const char* name = "posix_sem";
  unsigned int value = 0; 	/* 세마포어의 값 */  

  /* 세마포어 열기 */
  sem = sem_open(name, O_CREAT, S_IRUSR | S_IWUSR, value);  
  
  if((pid = fork()) < 0) { 	/* fork( ) 함수의 에러 시 처리 */
    perror("fork()");
    return -1;
  } 
  else if(pid == 0) { 	  /* 자식 프로세스인 경우의 처리 */
    for(int i = 0; i < 10; i++) {
      p();
    }
    exit(127);
  } 
  else { 			/* 부모 프로세스인 경우의 처리 */
    for(int i = 0; i < 10; i++) {
      sleep(1);
      v();
    }    
    waitpid(pid, &status, 0); 		/* 자식 프로세스의 종료를 기다리기 */
  }  /* 다 쓴 세마포어 닫고 정리 */
  
  sem_close(sem);
//  printf("sem_destroy return value : %d\n", sem_destroy(sem));  /* 세마포어 삭제 */
  
  sem_unlink(name);  

  return 0;
}
