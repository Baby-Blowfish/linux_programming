#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // 두 이미지를 컬러 형식으로 Mat 객체에 각각 저장
    Mat image1 = imread("mandrill.png", IMREAD_COLOR);
    Mat image2 = imread("Lenna.png", IMREAD_COLOR);

    // 두 이미지를 더하고 각 픽셀 값을 2로 나누어 평균을 구함
    // 이 결과로 두 이미지의 합성된 이미지를 생성
    auto image = (image1 + image2) / 2;

    // "Add Images"라는 창에 합성된 이미지를 표시
    imshow("Add Images", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);

    return 0; // 프로그램 종료
}
