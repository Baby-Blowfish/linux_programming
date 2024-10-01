#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{   
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);
    
    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 글자 색상(노란색)을 지정하는 Scalar 객체 생성 (B, G, R 형식으로 지정됨)
    Scalar color(255, 255, 0);
    
    // 글자의 크기 배율을 지정
    float scale  = 0.8;
    
    // 이미지에 다양한 글꼴의 텍스트를 그림
    putText(image, "FONT_HERSHEY_SIMPLEX", Point(10, 30), FONT_HERSHEY_SIMPLEX, scale, color, 1);         // 일반적인 산세리프 글꼴
    putText(image, "FONT_HERSHEY_PLAIN", Point(10, 60), FONT_HERSHEY_PLAIN, scale, color, 1);            // 간단하고 얇은 산세리프 글꼴
    putText(image, "FONT_HERSHEY_DUPLEX", Point(10, 90), FONT_HERSHEY_DUPLEX, scale, color, 1);          // 두꺼운 산세리프 글꼴
    putText(image, "FONT_HERSHEY_COMPLEX", Point(10, 120), FONT_HERSHEY_COMPLEX, scale, color, 1);       // 복잡한 산세리프 글꼴
    putText(image, "FONT_HERSHEY_TRIPLEX", Point(10, 150), FONT_HERSHEY_TRIPLEX, scale, color, 1);       // 세겹의 산세리프 글꼴
    putText(image, "FONT_HERSHEY_COMPLEX_SMAL", Point(10, 180), FONT_HERSHEY_COMPLEX_SMALL, scale, color, 1); // 복잡한 작은 글꼴
    putText(image, "FONT_HERSHEY_SCRIPT_SIMPLEX", Point(10, 210), FONT_HERSHEY_SCRIPT_SIMPLEX, scale, color, 1); // 간단한 필기체 글꼴
    putText(image, "FONT_HERSHEY_SCRIPT_COMPLEX", Point(10, 240), FONT_HERSHEY_SCRIPT_COMPLEX, scale, color, 1); // 복잡한 필기체 글꼴
    putText(image, "FONT_HERSHEY_PLAIN + FONT_ITALIC", Point(10, 270), FONT_HERSHEY_PLAIN | FONT_ITALIC, scale, color, 1); // 이탤릭체 적용한 간단한 글꼴
    
    // "Draw Text"라는 창에 이미지를 표시
    imshow("Draw Text", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);

    return 0; // 프로그램 종료
}
