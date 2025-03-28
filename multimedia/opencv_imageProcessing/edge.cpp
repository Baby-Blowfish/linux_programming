#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고급 GUI 기능을 위한 헤더 파일
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 위한 헤더 파일

using namespace cv;  // OpenCV의 네임스페이스를 사용

int main()
{
    // 그레이스케일로 이미지를 읽어옵니다.
    Mat image1 = imread("mandrill.bmp", IMREAD_GRAYSCALE);

    // 경계 검출 결과를 저장할 행렬을 초기화합니다.
    Mat image2 = Mat::zeros(image1.size(), image1.type()); // Sobel 필터 결과
    Mat image3 = Mat::zeros(image1.size(), image1.type()); // Laplacian 필터 결과
    Mat image4 = Mat::zeros(image1.size(), image1.type()); // Scharr 필터 결과
    Mat image5 = Mat::zeros(image1.size(), image1.type()); // Canny 필터 결과
    
    Mat xEdgeMat, yEdgeMat;  // x, y 방향의 경계 행렬
    int ddepth = CV_16S;     // 깊이(depth) 설정, 16비트 부호가 있는 정수

    // 가우시안 블러를 적용하여 이미지의 노이즈를 줄입니다.
    GaussianBlur(image1, image2, Size(3, 3), 0, 0, BORDER_DEFAULT);

    // Sobel 필터를 사용하여 x 방향의 경계를 검출합니다.
    Sobel(image1, xEdgeMat, ddepth, 1, 0);
    // Sobel 필터를 사용하여 y 방향의 경계를 검출합니다.
    Sobel(image2, yEdgeMat, ddepth, 0, 1);
    // x 방향 경계를 절대값으로 변환합니다.
    convertScaleAbs(xEdgeMat, xEdgeMat);
    // y 방향 경계를 절대값으로 변환합니다.
    convertScaleAbs(yEdgeMat, yEdgeMat);
    // x 방향과 y 방향 경계를 합쳐서 최종 경계 이미지를 생성합니다.
    addWeighted(xEdgeMat, 0.5, yEdgeMat, 0.5, 0, image2);

    // Laplacian 필터를 사용하여 경계를 검출합니다.
    Laplacian(image1, image3, ddepth, 3);
    // 결과를 절대값으로 변환합니다.
    convertScaleAbs(image3, image3);

    // Scharr 필터를 사용하여 x 방향의 경계를 검출합니다.
    Scharr(image1, xEdgeMat, ddepth, 1, 0);
    // Scharr 필터를 사용하여 y 방향의 경계를 검출합니다.
    Scharr(image2, yEdgeMat, ddepth, 0, 1);
    // x 방향 경계를 절대값으로 변환합니다.
    convertScaleAbs(xEdgeMat, xEdgeMat);
    // y 방향 경계를 절대값으로 변환합니다.
    convertScaleAbs(yEdgeMat, yEdgeMat);
    // x 방향과 y 방향 경계를 합쳐서 최종 경계 이미지를 생성합니다.
    addWeighted(xEdgeMat, 0.5, yEdgeMat, 0.5, 0, image4);

    // Canny 경계 검출을 사용하여 경계를 검출합니다.
    Canny(image1, image5, 50, 150);

    // 결과 이미지를 화면에 표시합니다.
    imshow("image", image1);
    imshow("Sobel", image2);
    imshow("Laplacian", image3);
    imshow("scharr", image4);
    imshow("Canny", image5);

    waitKey(0); // 키 입력 대기

    return 0; // 프로그램 종료
}
