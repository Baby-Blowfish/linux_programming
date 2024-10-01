#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // "mandrill.jpg" 이미지를 읽어와서 컬러 형식으로 Mat 객체에 저장
    // 이미지를 컬러로 읽어오기 위해 IMREAD_COLOR 옵션을 사용
    Mat image = imread("mandrill.jpg", IMREAD_COLOR);

    // 이미지의 모든 픽셀 값을 2배로 곱하여 밝기를 증가시킴
    // 이 연산은 각 채널의 픽셀 값에 대해 산술 연산을 수행하며, 결과값이 255를 초과하는 경우 자동으로 255로 클리핑됨
    image *= 2;
    
    // "Mat : Multiply"라는 창에 이미지를 표시
    imshow("Mat : Multiply", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
