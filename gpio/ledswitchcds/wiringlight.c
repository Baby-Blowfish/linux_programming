#include <wiringPi.h>
#include <stdio.h>

#define SW 	9 		/* GPIO. 9 */
#define LED 8 		/* GPIO. 8 */
#define CDS 0		/* GPIO. 0 */
int cdsControl( )
{
    int i;
	printf("a"); fflush(stdout);
    pinMode(SW, INPUT); 	/* Pin 모드를 입력으로 설정 */
    pinMode(CDS, INPUT); 	/* Pin 모드를 입력으로 설정 */
    pinMode(LED, OUTPUT); 	/* Pin 모드를 출력으로 설정 */

    for (;;) { 			/* 조도 센서 검사를 위해 무한 루프를 실행한다. */
        if(digitalRead(CDS) == HIGH) { 	/* 빛이 감지되면(HIGH) */
            printf("e"); fflush(stdout);
			digitalWrite(LED, HIGH); 	/* LED 켜기(On) */
            delay(1000);
            digitalWrite(LED, LOW); 	/* LED 끄기(Off) */
        }
    }

    return 0;
}

int main( )
{
	printf("c"); fflush(stdout);
    wiringPiSetup( );
	printf("a"); fflush(stdout);
    cdsControl( ); 		/* 조도 센서 사용을 위한 함수 호출 */
    return 0;
}
