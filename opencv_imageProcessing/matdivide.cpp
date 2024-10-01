#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // "mandrill.jpg" 이미지를 읽어와서 컬러 형식으로 Mat 객체에 저장
    Mat image = imread("mandrill.jpg", IMREAD_COLOR);

    // 이미지의 모든 픽셀 값을 2로 나누어 어둡게 만듦
    // 각 채널의 픽셀 값에 대해 산술 연산을 수행
    image /= 2;

    // "Mat : Divide"라는 창에 이미지를 표시
    imshow("Mat : Divide", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);

    return 0; // 프로그램 종료
}
