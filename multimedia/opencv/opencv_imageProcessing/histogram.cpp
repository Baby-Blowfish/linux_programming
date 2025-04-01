#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    MatND hist;  // 히스토그램 데이터를 저장할 객체
    const int* chan_nos = {0}; // 계산할 채널 번호 (0: 그레이스케일 또는 B 채널)
    float chan_range[] = {0.0, 255.0}; // 히스토그램 계산 범위 (0~255)
    const float* chan_ranges = chan_range; // 히스토그램 계산에 사용할 범위 포인터
    int histSize = 255; // 히스토그램의 크기 (빈의 개수)
    double maxVal = 0, minVal = 0; // 히스토그램의 최대값과 최소값을 저장할 변수

    // 그레이스케일로 이미지 읽어오기
    Mat image1  = imread("mandrill.jpg", IMREAD_GRAYSCALE);

    // image1과 같은 크기와 타입의 검정색 이미지 생성
    Mat image2  = Mat::zeros(image1.size(), image1.type());

    // 히스토그램을 그릴 이미지 생성, 초기값은 밝은 회색 (255)
    Mat histImg1(histSize, histSize, CV_8U, Scalar(histSize));
    Mat histImg2(histSize, histSize, CV_8U, Scalar(histSize));

    // image1의 밝기를 증가시킨 이미지를 image2에 저장
    image2 = image1 + 50;

    // 첫 번째 이미지의 히스토그램을 계산
    calcHist(&image1, 1, chan_nos, Mat(), hist, 1, &histSize, &chan_ranges);

    // 히스토그램의 최소값과 최대값을 계산
    minMaxLoc(hist, &minVal, &maxVal, 0, 0);

    // 히스토그램을 그릴 최대 높이 설정
    int hpt = static_cast<int>(0.9 * histSize);

    // 첫 번째 이미지의 히스토그램을 그리기
    for(int h = 0; h < histSize; h++)
    {
        float binVal = hist.at<float>(h); // 해당 빈의 값
        int intensity = static_cast<int>(binVal * hpt / maxVal); // 히스토그램 높이를 계산

        // 히스토그램 이미지를 그리는 부분 (세로 선 그리기)
        line(histImg1, Point(h, histSize), Point(h, histSize - intensity), Scalar::all(0));
    }

    // 두 번째 이미지의 히스토그램을 계산
    calcHist(&image2, 1, chan_nos, Mat(), hist, 1, &histSize, &chan_ranges);

    // 히스토그램의 최소값과 최대값을 계산
    minMaxLoc(hist, &minVal, &maxVal, 0, 0);

    // 두 번째 이미지의 히스토그램을 그리기
    for(int h = 0; h < histSize; h++)
    {
        float binVal = hist.at<float>(h); // 해당 빈의 값
        int intensity = static_cast<int>(binVal * hpt / maxVal); // 히스토그램 높이를 계산

        // 히스토그램 이미지를 그리는 부분 (세로 선 그리기)
        line(histImg2, Point(h, histSize), Point(h, histSize - intensity), Scalar::all(0));
    }

    // 두 개의 히스토그램을 화면에 표시
    imshow("Histogram1", histImg1);
    imshow("Histogram2", histImg2);

    // 키 입력이 있을 때까지 대기
    waitKey(0);

    return 0; // 프로그램 종료
}
