#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    int histSize = 255; // 히스토그램 이미지를 표시할 크기
    // 컬러 이미지를 읽어와 Mat 객체에 저장
    Mat image1 = imread("mandrill.jpg", IMREAD_COLOR);

    // image1과 같은 크기와 타입으로 검정색 이미지 생성
    Mat image2 = Mat::zeros(image1.size(), image1.type());

    // 히스토그램 개선 이미지를 저장할 Mat 객체를 생성하고 밝은 회색으로 초기화
    Mat histImg(histSize, histSize, CV_8U, Scalar(histSize));

    // 원본 이미지의 밝기를 50만큼 증가시킴
    image1 += 50;

    // BGR 형식의 이미지를 YCrCb 색 공간으로 변환 (밝기 채널 분리하기 위해)
    cvtColor(image1, image2, COLOR_BGR2YCrCb);

    // 채널을 분리하여 벡터에 저장
    std::vector<Mat> channels;
    split(image2, channels);

    // 첫 번째 채널(Y, 밝기 채널)의 히스토그램을 평활화
    equalizeHist(channels[0], channels[0]);

    // 평활화된 채널들을 다시 합쳐서 YCrCb 이미지 생성
    merge(channels, image2);

    // YCrCb 이미지를 다시 BGR로 변환하여 결과 이미지를 생성
    cvtColor(image2, histImg, COLOR_YCrCb2BGR);

    // 원본 이미지와 히스토그램 평활화 결과 이미지를 창에 표시
    imshow("Image", image1);       // 밝기가 증가된 원본 이미지 표시
    imshow("equalize", histImg);   // 히스토그램 평활화된 이미지 표시

    // 키 입력을 대기 (키 입력이 있을 때까지 창이 유지됨)
    waitKey(0);

    return 0; // 프로그램 종료
}
