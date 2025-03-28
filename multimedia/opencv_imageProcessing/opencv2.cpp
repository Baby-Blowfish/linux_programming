#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main(int argc, char **argv)
{
    // lenna.png와 mandrill.png 이미지를 컬러 형식으로 Mat 객체에 각각 불러옴
    Mat image1 = imread("sample1.jpg", IMREAD_COLOR);
    Mat image2 = imread("sample2.jpg", IMREAD_COLOR);

    // 이미지 연산 결과를 저장할 Mat 객체를 생성하고 초기화 (모두 0으로 설정)
    Mat image_add = Mat::zeros(image1.size(), image1.type());   // 덧셈 결과를 저장할 Mat 객체
    Mat image_sub = Mat::zeros(image1.size(), image1.type());   // 뺄셈 결과를 저장할 Mat 객체
    Mat image_mul = Mat::zeros(image1.size(), image1.type());   // 곱셈 결과를 저장할 Mat 객체
    Mat image_div = Mat::zeros(image1.size(), image1.type());   // 나눗셈 결과를 저장할 Mat 객체
    Mat image_gray1 = Mat::zeros(image1.size(), CV_8UC1);       // 첫 번째 이미지를 그레이스케일로 변환하여 저장할 Mat 객체
    Mat image_gray2 = Mat::zeros(image1.size(), CV_8UC1);       // 두 번째 이미지를 그레이스케일로 변환하여 저장할 Mat 객체
    Mat image_white = Mat::zeros(image1.size(), CV_8UC1);       // 이진화 결과를 저장할 Mat 객체
    Mat image_gray_sub = Mat::zeros(image1.size(), CV_8UC1);    // 두 그레이스케일 이미지의 차이 결과를 저장할 Mat 객체

    // 이미지 연산 수행
    add(image1, image2, image_add);                            // 두 이미지를 더함
    subtract(image1, image2, image_sub);                       // 두 이미지를 뺌
    multiply(image1, image2, image_mul);                       // 두 이미지를 곱함
    divide(image1, image2, image_div);                         // 두 이미지를 나눔
    cvtColor(image1, image_gray1, COLOR_RGB2GRAY);             // 첫 번째 이미지를 그레이스케일로 변환
    cvtColor(image2, image_gray2, COLOR_RGB2GRAY);             // 두 번째 이미지를 그레이스케일로 변환
    absdiff(image_gray1, image_gray2, image_gray_sub);         // 두 그레이스케일 이미지의 절대 차이 계산
    threshold(image_gray_sub, image_white, 100, 255, THRESH_BINARY); // 이진화 처리 (threshold 100 이상이면 255로 설정)

    // 각 이미지를 창에 표시
    imshow("IMAGE_1", image1);             // 원본 이미지 1 표시
    imshow("IMAGE_2", image2);             // 원본 이미지 2 표시
    imshow("IMAGE_ADDTION", image_add);    // 이미지 덧셈 결과 표시
    imshow("IMAGE_SUBTRACTION", image_sub); // 이미지 뺄셈 결과 표시
    imshow("IMAGE_MULTIPLICATION", image_mul); // 이미지 곱셈 결과 표시
    imshow("IMAGE_DIVISION", image_div);   // 이미지 나눗셈 결과 표시
    imshow("IMAGE_GRAY1", image_gray1);    // 첫 번째 이미지의 그레이스케일 변환 결과 표시
    imshow("IMAGE_GRAY2", image_gray2);    // 두 번째 이미지의 그레이스케일 변환 결과 표시
    imshow("IMAGE_WHITE", image_white);    // 이진화된 이미지 표시
    
    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);

    // 모든 창을 닫음
    destroyAllWindows();

    return 0; // 프로그램 종료
}
