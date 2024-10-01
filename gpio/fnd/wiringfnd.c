#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

int fndControl(int num)
{
    int i;
    int gpiopins[7] = {1, 2, 3, 5, 6, 7, 8}; 	/* E, D, C, B, A, F, G */
    int number[10][7] = { {0,0,0,0,0,0,1}, 	/* 0 */
     		  {1,1,0,0,1,1,1}, 	/* 1 */
			  {0,0,1,0,0,1,0}, 	/* 2 */
   			  {1,0,0,0,0,1,0}, 	/* 3 */
			  {1,1,0,0,1,0,0}, 	/* 4 */
			  {1,0,0,1,0,0,0}, 	/* 5 */
			  {0,0,0,1,1,0,0}, 	/* 6 */
			  {1,1,0,0,0,1,1}, 	/* 7 */
			  {0,0,0,0,0,0,0}, 	/* 8 */
			  {1,1,0,0,0,0,0} };/* 9 */
    for (i = 0; i < 7; i++) {
        pinMode(gpiopins[i], OUTPUT); 	/* 모든 Pin의 출력 설정 */
    }

    for (i = 0; i < 7; i++) {
        digitalWrite(gpiopins[i], number[num][i]?HIGH:LOW); 	/* FND에 값을 출력 */
    }

    getchar( ); 			/* 숫자 표시 대기 */

    for(int i = 0; i < 7; i++) { 	/* FND 초기화 */
        digitalWrite(gpiopins[i], HIGH);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int no;
    if(argc < 2) {
        printf("Usage : %s NO\n", argv[0]);
        return -1;
    }

    no = atoi(argv[1]);
    wiringPiSetup( ); 			/* wiringPi 초기화 */
    fndControl(no);

    return 0;
}
