#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{   
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);
    
    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 선의 색상(파란색)을 지정하는 Scalar 객체 생성 (B, G, R 형식으로 지정됨)
    Scalar color(255, 0, 0);
    
    // 시작점과 끝점을 지정하는 Point 객체 생성
    Point p1(10, 10);  // 시작점 좌표 (10, 10)
    Point p2(100, 100); // 끝점 좌표 (100, 100)
    
    // 지정된 색상과 두께(5픽셀)로 p1에서 p2까지 선을 그림
    line(image, p1, p2, color, 5);
    
    // "Draw Line"이라는 창에 이미지를 표시
    imshow("Draw Line", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
