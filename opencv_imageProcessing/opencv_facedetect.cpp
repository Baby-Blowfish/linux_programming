#include <stdlib.h>
#include <fcntl.h>               // 파일 제어를 위한 헤더
#include <unistd.h>              // UNIX 표준 함수들을 위한 헤더
#include <linux/fb.h>            // 프레임버퍼를 위한 헤더
#include <sys/mman.h>            // 메모리 매핑을 위한 헤더
#include <sys/ioctl.h>           // 입출력 제어를 위한 헤더
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp> // OpenCV 고급 GUI 관련 헤더
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>


using namespace cv;              // OpenCV 네임스페이스 사용

#define FBDEV "/dev/fb0"         // 프레임버퍼 장치 경로 정의
#define CAMERA_COUNT 100         // 캡처할 카메라 프레임 수
#define CAM_WIDTH 640            // 카메라 너비
#define CAM_HEIGHT 480           // 카메라 높이


const static char* cascade_name = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml"; // 얼굴 인식을 위한 Haar Cascade 파일 경로

// 16비트 픽셀 생성 함수
// RGB 값을 받아 16비트의 픽셀 데이터로 변환
unsigned short makePixel(uchar r, uchar g, uchar b) {
    // R: 상위 5비트, G: 중간 6비트, B: 하위 5비트로 변환
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

int main(int argc, char **argv)
{
    int fbfd;                     // 프레임버퍼 파일 디스크립터
    struct fb_var_screeninfo vinfo; // 프레임버퍼 가변 화면 정보 구조체

    unsigned char *buffer, *pfbmap; // 프레임버퍼와 영상 데이터를 담을 버퍼
    unsigned int x, y, i, j, screensize; // 반복문과 화면 크기 계산용 변수들

    // 카메라 객체 생성 (장치 번호 0번 카메라)
	VideoCapture vc(0, CAP_V4L2);

	// 얼굴 탐지를 위한 분류기 객체 생성
	CascadeClassifier cascade;		

	// 탐지된 얼굴의 두 점
	Point pt1, pt2;

    // 카메라로부터 이미지를 저장할 Mat 객체 생성, 초기값은 흰색(255)
    Mat image(CAM_WIDTH, CAM_HEIGHT, CV_8UC3, Scalar(255));

	// Haar Cascade 파일 로드
	if(!cascade.load(cascade_name))
	{
		perror("load()"); // 로드 실패 시 에러 출력
		return EXIT_FAILURE;
	}

    // 카메라가 정상적으로 열리지 않았을 경우 오류 처리
    if (!vc.isOpened())
    {
        perror("OpenCV: open WebCam\n"); // 카메라 열기 실패 시 에러 출력
        return EXIT_FAILURE;
    }

    // 카메라 해상도 설정
    vc.set(CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
    vc.set(CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);

    // 프레임버퍼 장치 열기 (읽기/쓰기 모드)
    fbfd = open(FBDEV, O_RDWR);
    if (fbfd == -1)  // 정상적으로 열리지 않았을 경우 오류 처리
    {
        perror("open() : framebuffer device"); // 에러 출력
        return EXIT_FAILURE;
    }

    // 프레임버퍼의 화면 정보 가져오기
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information."); // 에러 출력
        return EXIT_FAILURE;
    }

    // 화면 크기 계산 (가로 * 세로 * 비트 수 / 바이트 단위로 변환)
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    
    // 프레임버퍼 메모리 맵핑
    pfbmap = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (pfbmap == MAP_FAILED) // 맵핑이 실패했을 경우 오류 처리
    {
        perror("mmap() : framebuffer device to memory"); // 에러 출력
        return EXIT_FAILURE;
    }
    
    // 화면을 검은색으로 초기화
    memset(pfbmap, 0, screensize);

    // 카메라로부터 100개의 프레임을 캡처하여 처리
    for (i = 0; i < CAMERA_COUNT; i++)
    {
        int colors = vinfo.bits_per_pixel / 8;  // 픽셀당 바이트 수 계산
        long location = 0;                      // 프레임버퍼 위치 계산용 변수
        int istride = image.cols * colors;      // 한 행의 바이트 수

		// 유효한 프레임이 캡처될 때까지 반복
		do
		{
			usleep(1000); // 1ms 대기
		    vc >> image;  // 프레임 캡처
		}
		while (image.empty()); // 프레임이 비었으면 다시 프레임을 받을 때까지 반복
		   

		Mat image1(CAM_WIDTH, CAM_HEIGHT, CV_8UC1, Scalar(255)); // 그레이스케일 이미지를 위한 Mat 객체 생성
		
		cvtColor(image, image1, COLOR_BGR2GRAY); // 컬러 이미지를 그레이스케일로 변환
		
		equalizeHist(image1, image1); // 히스토그램 평활화로 대비 향상
		
		std::vector<Rect> faces; // 탐지된 얼굴의 사각형을 저장할 벡터
		cascade.detectMultiScale(image1, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30)); // 얼굴 탐지

		// 탐지된 얼굴 사각형 그리기
		for(j = 0; j < faces.size(); j++)
		{
			pt1.x = faces[j].x; pt2.x = (faces[j].x + faces[j].width); // 사각형의 오른쪽 아래 점
			pt1.y = faces[j].y; pt2.y = (faces[j].y + faces[j].height); // 사각형의 왼쪽 위 점

			rectangle(image, pt1, pt2, Scalar(255, 0, 0), 3, 8); // 이미지에 얼굴 사각형 그리기
		}

		buffer = (unsigned char*)image.data;  // 캡처한 이미지 데이터를 buffer에 저장

		// 이미지 데이터를 프레임버퍼에 출력
		for (y = 0, location = 0; y < image.rows; y++)
		{
			for (x = 0; x < vinfo.xres; x++)
			{
				// 이미지가 프레임버퍼 너비보다 작을 경우 나머지 부분은 검정색으로 채움
				if (x >= image.cols)
				{
					*(unsigned short *)(pfbmap + location) = 0; // 검정색으로 처리
					location += sizeof(unsigned short);
					continue;
				}

				// 16비트 픽셀 생성 (BGR 순서로 변환)
				unsigned short pixel = makePixel(buffer[(y * image.cols + x) * 3 + 2], // R
												 buffer[(y * image.cols + x) * 3 + 1], // G
												 buffer[(y * image.cols + x) * 3 + 0]); // B

				// 프레임버퍼에 픽셀 값 쓰기
				*(unsigned short *)(pfbmap + location) = pixel;
				location += sizeof(unsigned short); // 다음 위치로 이동
			}
			
			// 각 행이 끝날 때 다음 줄로 이동 (프레임버퍼 너비 고려)
			location = (y + 1) * vinfo.xres * sizeof(unsigned short);
		}

    }

    // 메모리 맵핑 해제
    munmap(pfbmap, screensize);
	
	vc.release(); // 카메라 리소스 해제
	
    // 프레임버퍼 장치 닫기
    close(fbfd);

    return 0; // 프로그램 정상 종료
}
