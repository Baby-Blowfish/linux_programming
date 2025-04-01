#include <opencv2/highgui/highgui.hpp>  // OpenCV의 고수준 GUI 기능을 사용하기 위한 헤더 파일 포함
#include <opencv2/imgproc/imgproc.hpp>  // OpenCV의 이미지 처리 기능을 사용하기 위한 헤더 파일 포함

using namespace cv; // OpenCV의 cv 네임스페이스를 사용하여 코드 내에서 cv:: 없이 함수와 클래스를 사용

// 현재 그림을 그리는 중인지 여부를 나타내는 변수 (0: 그리지 않는 상태, 1: 그리는 중)
int isDrawing = 0; 

// 이전 마우스 위치를 저장하는 변수
Point prevPt;

// 이미지를 저장하는 Mat 객체
Mat image; 

// 마우스 콜백 함수
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    // 그릴 선과 점의 색상을 지정 (검은색)
    Scalar color(0, 0, 0);

    // 현재 마우스 위치를 Point 객체로 생성
    Point newPt(x, y);

    // 왼쪽 마우스 버튼이 눌렸을 때
    if (event == EVENT_LBUTTONDOWN)
    {
        isDrawing = 1; // 그림을 그리는 상태로 변경
        circle(image, newPt, 1, color, -1); // 현재 위치에 작은 점을 그림
    }
    // 마우스가 움직일 때
    else if (event == EVENT_MOUSEMOVE)
    {
        if (isDrawing) // 그림을 그리는 상태일 때만
        {
            line(image, prevPt, newPt, color, 1); // 이전 위치와 현재 위치 사이에 선을 그림
        }
    }
    // 왼쪽 마우스 버튼이 떼어졌을 때
    else if (event == EVENT_LBUTTONUP)
    {
        isDrawing = 0; // 그림을 그리지 않는 상태로 변경
        line(image, prevPt, newPt, color, 1); // 마지막 선을 그림
    }

    // 현재 마우스 위치를 prevPt에 저장하여 다음 선을 그릴 때 사용
    prevPt = newPt;
}

int main()
{
    // 300x400 크기의 3채널(RGB) 이미지를 생성하고 모든 픽셀 값을 흰색으로 초기화
    image = Mat(300, 400, CV_8UC3, Scalar(255, 255, 255));

    // "Draw"라는 이름의 창을 생성 (크기 조절 가능)
    namedWindow("Draw", WINDOW_NORMAL);

    // 마우스 콜백 함수를 설정하여 "Draw" 창에서 마우스 이벤트를 처리
    setMouseCallback("Draw", CallBackFunc, NULL);

    // 무한 루프를 통해 이미지를 계속 화면에 표시
    while (1)
    {
        imshow("Draw", image); // "Draw" 창에 이미지를 표시
        int key = waitKey(1) & 0xFF; // 1밀리초 동안 키 입력 대기 (반환된 값을 0xFF와 비트 연산하여 1바이트로 제한)
        if (key == 27) break; // ESC 키를 누르면 루프를 빠져나감
    }

    // 모든 창을 닫음
    destroyAllWindows();

    return 0; // 프로그램 종료
}
