# Linux Multimedia Programming Examples

이 프로젝트는 Linux 시스템에서의 멀티미디어 프로그래밍, 비디오 캡처, 이미지 처리, 프레임버퍼 조작 등에 대한 예제들을 포함하고 있습니다.

## 디렉토리 구조

### 1. V4L2 (Video4Linux2) 프로그래밍 (v4l2/)
- `v4l2_info.c`: 비디오 장치 정보 확인
  - VIDIOC_QUERYCAP으로 장치 기능 확인
  - 지원하는 픽셀 포맷 열거
  - 지원하는 해상도 확인
  - 드라이버 및 버스 정보 출력

- `v4l2_format_set.c`: 비디오 포맷 설정
  - VIDIOC_S_FMT으로 포맷 설정
  - 해상도 및 픽셀 포맷 변경
  - 프레임 레이트 설정
  - 버퍼 설정

- `v4l2_framebuffer.c`: 비디오 캡처 및 프레임버퍼 표시
  - 비디오 스트림 캡처
  - 메모리 매핑 버퍼 사용
  - 프레임버퍼에 직접 출력
  - 실시간 비디오 처리

- `v4l2_framebuffer_laptop.c`: 노트북 카메라용 프레임버퍼 처리
  - 노트북 내장 카메라 지원
  - YUV/RGB 변환
  - 화면 회전 처리
  - 성능 최적화

### 2. OpenCV 프로그래밍 (opencv/)
- `opencv_basic/`: OpenCV 기본 예제
  - 이미지 로드 및 저장
  - 기본 이미지 처리
  - 윈도우 생성 및 관리
  - 마우스/키보드 이벤트 처리

- `opencv_imageProcessing/`: 이미지 처리 예제
  - 필터링 및 변환
  - 엣지 검출
  - 모폴로지 연산
  - 컬러 변환

- `gstreamer/`: GStreamer 통합
  - GStreamer 파이프라인 구성
  - OpenCV와 GStreamer 연동
  - 실시간 비디오 처리
  - 스트리밍 처리

### 3. 프레임버퍼 프로그래밍 (framebuffer/)
- `fbinfo.c`: 프레임버퍼 정보 확인
  - FBIOGET_FSCREENINFO로 고정 정보 확인
  - FBIOGET_VSCREENINFO로 가변 정보 확인
  - 해상도 및 색상 정보 출력
  - 프레임버퍼 메모리 정보

- `draw_laptop.c`: 프레임버퍼 직접 그리기
  - 점, 선, 원 그리기
  - 색상 처리
  - 좌표 변환
  - 메모리 매핑 사용

- `fbdrawmmap.c`: 메모리 매핑을 이용한 그리기
  - mmap()으로 프레임버퍼 매핑
  - 직접 메모리 접근
  - 더블 버퍼링
  - 성능 최적화

- `bmp/`: BMP 파일 처리
  - BMP 파일 로드
  - 프레임버퍼에 BMP 표시
  - 이미지 변환
  - 파일 입출력

## 주요 개념

### V4L2 (Video4Linux2)
- 장치 제어
  - ioctl() 시스템 콜
  - VIDIOC_QUERYCAP
  - VIDIOC_S_FMT
  - VIDIOC_REQBUFS

- 버퍼 관리
  - 메모리 매핑
  - 사용자 포인터
  - DMA 버퍼
  - 스트리밍 I/O

- 포맷 및 해상도
  - 픽셀 포맷 (YUYV, RGB, MJPEG)
  - 해상도 설정
  - 프레임 레이트
  - 필드 타입

### OpenCV
- 이미지 처리
  - Mat 클래스
  - 필터링
  - 변환
  - 특징점 검출

- 비디오 처리
  - VideoCapture
  - VideoWriter
  - 프레임 처리
  - 실시간 처리

- GUI
  - 윈도우 관리
  - 이벤트 처리
  - 트랙바
  - 마우스 콜백

### 프레임버퍼
- 장치 제어
  - /dev/fb0 장치
  - ioctl() 시스템 콜
  - FBIOPUT_VSCREENINFO
  - FBIOGET_VSCREENINFO

- 메모리 관리
  - mmap() 시스템 콜
  - 직접 메모리 접근
  - 더블 버퍼링
  - 메모리 정렬

- 그래픽 처리
  - 픽셀 조작
  - 색상 변환
  - 좌표계 변환
  - 성능 최적화

## 참고사항
- 대부분의 예제는 Linux 환경에서 실행되어야 합니다.
- V4L2 예제는 비디오 장치가 필요합니다.
- 프레임버퍼 예제는 root 권한이 필요할 수 있습니다.
- OpenCV 예제는 OpenCV 라이브러리가 설치되어 있어야 합니다.
- GStreamer 예제는 GStreamer 라이브러리가 설치되어 있어야 합니다. 