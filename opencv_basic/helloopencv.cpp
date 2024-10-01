#include <opencv2/highgui/highgui.hpp> // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{   
    // 300x400 크기의 단일 채널(흑백) 이미지 행렬 생성, 모든 픽셀을 값 255(흰색)로 초기화
    Mat image(300, 400, CV_8UC1, Scalar(255)); 
    
    // "hello world"라는 창에 이미지 표시
    imshow("hello world", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
