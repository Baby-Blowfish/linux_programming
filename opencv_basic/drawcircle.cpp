#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);
    
    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 빨간색(0, 0, 255) 색상을 지정하는 Scalar 객체 생성 (B, G, R 형식)
    Scalar color(0, 0, 255);
    
    // 원과 타원을 그릴 두 개의 중심점 좌표 지정
    Point p1(100, 100); // (100, 100) 위치에 원을 그릴 중심점
    Point p2(220, 100); // (220, 100) 위치에 타원을 그릴 중심점
    
    // 타원의 크기(가로 50, 세로 40)를 지정하는 Size 객체 생성
    Size size(50, 40);
    
    // p1을 중심으로 반지름 50의 빨간색 채워진 원을 그림 (-1은 내부를 채움)
    circle(image, p1, 50, color, -1);
    
    // p2를 중심으로 크기 size(50x40)의 타원을 그리는데, 
    // 45도 각도로 회전하고, 시작 각도 0부터 270도까지 빨간색으로 채움
    ellipse(image, p2, size, 45, 0, 270, color, -1);
    
    // "Draw circle"이라는 창에 이미지를 표시
    imshow("Draw circle", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
