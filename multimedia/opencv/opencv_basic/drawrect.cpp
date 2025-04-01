#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);
    
    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 녹색(0, 255, 0) 색상을 지정하는 Scalar 객체 생성 (B, G, R 형식)
    Scalar color(0, 255, 0);
    
    // 2D 공간의 3개의 좌표 (시작점과 끝점, p3는 사각형 생성용) 정의
    Point p1(10, 10);   // (10, 10) 위치
    Point p2(100, 100); // (100, 100) 위치
    Point p3(220, 10);  // (220, 10) 위치
    
    // Rect를 생성하기 위한 크기(size)를 지정 (100x100 크기)
    Size size(100, 100);
    
    // 좌표와 크기를 이용하여 두 개의 Rect 객체 생성
    Rect rect1(110, 10, 100, 100); // (110, 10) 위치에 가로 100, 세로 100 크기의 사각형
    Rect rect2(p3, size);          // p3(220, 10)를 시작점으로 하고, `size` 크기(100x100)의 사각형
    
    // 좌표 p1에서 p2까지 녹색 선(두께 2픽셀)으로 사각형을 그립니다
    rectangle(image, p1, p2, color, 2);
    
    // rect1을 이미지에 그립니다 (녹색, 두께 2픽셀)
    rectangle(image, rect1, color, 2);
    
    // rect2를 이미지에 그립니다 (녹색, 두께 2픽셀)
    rectangle(image, rect2, color, 2);
    
    // "Draw Rect"라는 창에 이미지를 표시
    imshow("Draw Rect", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
