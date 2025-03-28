#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);

    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 색상(파란색, BGR 형식)을 지정하는 Scalar 객체 생성 (B=255, G=255, R=0, 노란색으로 잘못 표현되어 있음)
    Scalar color(255, 255, 0); 

    // JPEG와 PNG 이미지 저장 시 사용될 파라미터를 저장할 벡터 생성
    std::vector<int> jpegParams, pngParams;
    
    // JPEG 이미지의 품질을 80으로 설정 (기본값은 95)
    jpegParams.push_back(IMWRITE_JPEG_QUALITY);
    jpegParams.push_back(80);
    
    // PNG 이미지의 압축 레벨을 9로 설정 (0은 압축 없음, 9는 최대 압축)
    pngParams.push_back(IMWRITE_PNG_COMPRESSION);
    pngParams.push_back(9);
    
    // (100, 100) 위치에 반지름이 50인 노란색 채워진 원을 그림
    circle(image, Point(100, 100), 50, color, -1);

    // JPEG 형식으로 이미지 저장, 지정된 품질 파라미터 사용
    imwrite("sample.jpg", image, jpegParams);
    
    // PNG 형식으로 이미지 저장, 지정된 압축 파라미터 사용
    imwrite("sample.png", image, pngParams);

    return 0; // 프로그램 종료
}
