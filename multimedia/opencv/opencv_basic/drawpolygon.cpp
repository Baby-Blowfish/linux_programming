#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

int main()
{
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 0(검은색)으로 초기화
    Mat image = Mat::zeros(300, 400, CV_8UC3);
    
    // 이미지의 모든 픽셀 값을 (255,255,255)로 설정하여 이미지를 흰색으로 만듦
    image.setTo(Scalar(255, 255, 255));
    
    // 선의 색상(보라색)을 지정하는 Scalar 객체 생성 (B, G, R 형식으로 지정됨)
    Scalar color(255, 0, 255);
    
    // 시작점과 끝점을 지정하는 Point 객체 생성
    Point p1(50, 50);    // 시작점 좌표 (50, 50)
    Point p2(150, 150);  // 끝점 좌표 (150, 150)
    
    // 다각형을 그리기 위한 포인트들을 저장할 벡터 생성
    std::vector<Point> contour;
    contour.push_back(p1);              // 첫 번째 점 추가 (50, 50)
    contour.push_back(Point(200, 100)); // 두 번째 점 추가 (200, 100)
    contour.push_back(Point(250, 50));  // 세 번째 점 추가 (250, 50)
    contour.push_back(Point(180, 200)); // 네 번째 점 추가 (180, 200)
    contour.push_back(p2);              // 다섯 번째 점 추가 (150, 150)
    
    // 다각형을 그리기 위한 점 포인터를 가져옴
    const Point *pts = (const cv::Point*) Mat(contour).data; // contour 데이터를 Point 배열로 변환
    int npts = contour.size(); // 다각형을 이루는 점의 개수
    
    
	//polylines(image, &pts, &npts, 1, true, color);
	polylines(image, &pts, &npts, 1, false, color);
	// 다각형을 채우기 위한 fillPoly 함수 호출
    //fillPoly(image, &pts, &npts, 1, color); // 다각형을 보라색으로 채움
    
    // "Draw Polygon"이라는 창에 이미지를 표시
    imshow("Draw Polygon", image);

    // 키 입력이 있을 때까지 프로그램 대기
    waitKey(0);

    return 0; // 프로그램 종료
}
