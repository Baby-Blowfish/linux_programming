#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함
#include <iostream>
using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{ 
    // "sample.jpg" 이미지를 읽어와서 3채널(RGB) 컬러 이미지로 Mat 객체에 저장
    // IMREAD_COLOR 옵션을 사용하여 이미지를 컬러로 불러옴 (이미지가 흑백인 경우에도 3채널로 변환)
    Mat image = imread("sample.jpg", IMREAD_COLOR);
    if (image.empty()) {
	std::cout << "Image file could not be loaded." << std::endl;
	return -1;
    }


    // "Load Image"라는 창에 이미지를 표시
    imshow("Load Image", image);
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);
    
    return 0; // 프로그램 종료
}
