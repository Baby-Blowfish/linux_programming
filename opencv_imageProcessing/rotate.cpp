#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // "mandrill.jpg" 이미지를 컬러 형식으로 읽어와 Mat 객체에 저장
    Mat image1 = imread("mandrill.jpg", IMREAD_COLOR);

    // image1과 같은 크기와 타입으로 검정색 이미지 생성 (각 변환 결과를 저장할 Mat 객체 생성)
    Mat image2 = Mat::zeros(image1.size(), image1.type()); // 회전 변환 결과
    Mat image3 = Mat::zeros(image1.size(), image1.type()); // 수직(상하) 대칭 변환 결과
    Mat image4 = Mat::zeros(image1.size(), image1.type()); // 수평(좌우) 대칭 변환 결과
    Mat image5 = Mat::zeros(image1.size(), image1.type()); // 전치 행렬 변환 결과

    // 회전 각도를 45도로 설정
    double angle = 45;

    // 회전의 중심점을 이미지의 중앙으로 설정
    Point2f center(image1.cols / 2., image1.rows / 2);

    // 2D 회전 변환 행렬을 계산 (중심점, 회전 각도, 스케일)
    Mat rotMat = getRotationMatrix2D(center, angle, 1.0);

    // 아핀 변환을 사용하여 이미지를 45도 회전
    warpAffine(image1, image2, rotMat, image1.size());

    // 이미지의 수직(상하) 대칭을 적용
    flip(image1, image3, 0);

    // 이미지의 수평(좌우) 대칭을 적용
    flip(image1, image4, 1);

    // 이미지의 전치 행렬을 계산 (행과 열을 뒤바꿈)
    transpose(image1, image5);

    // 원본 이미지와 변환된 이미지들을 각각의 창에 표시
    imshow("Image", image1);       // 원본 이미지
    imshow("warpAffine", image2);  // 45도 회전된 이미지
    imshow("flip", image3);        // 수직(상하) 대칭된 이미지
    imshow("mirror", image4);      // 수평(좌우) 대칭된 이미지
    imshow("transpose", image5);   // 전치 행렬 이미지

    // 키 입력이 있을 때까지 대기 (창이 유지됨)
    waitKey(0);

    return 0; // 프로그램 종료
}
