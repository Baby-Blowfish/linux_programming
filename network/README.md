# Linux Network Programming Examples

이 프로젝트는 Linux 시스템에서의 네트워크 프로그래밍, 소켓 통신, 웹 서버 구현, 데몬 프로세스 등에 대한 예제들을 포함하고 있습니다.

## 디렉토리 구조

### 1. TCP 통신 (tcp/)
- `echo_server.c`, `echo_client.c`: 기본 에코 서버/클라이언트
  - socket(), bind(), listen(), accept() 함수 사용
  - 클라이언트 연결 처리 및 데이터 에코
  - 소켓 옵션 설정 (SO_REUSEADDR)

- `multi_echo_fork_server.c`, `multi_echo_fork_client.c`: 멀티프로세스 에코 서버
  - fork()를 사용한 다중 클라이언트 처리
  - 부모-자식 프로세스 관계
  - 좀비 프로세스 방지

- `multi_echo_thread_server.c`, `multi_echo_thread_client.c`: 멀티스레드 에코 서버
  - pthread를 사용한 다중 클라이언트 처리
  - 스레드 생성 및 관리
  - 스레드 동기화

- `select_server.c`, `select_client.c`: select() 기반 다중 클라이언트 처리
  - select() 함수로 다중 소켓 모니터링
  - 비동기 I/O 처리
  - 타임아웃 설정

- `epoll_server.c`, `epoll_client.c`: epoll 기반 다중 클라이언트 처리
  - epoll API를 사용한 고성능 I/O 다중화
  - 이벤트 기반 처리
  - Linux 특화 기능 활용

- `tcp_server_chat.c`, `tcp_client_chat.c`: 채팅 서버/클라이언트
  - 다중 클라이언트 간 메시지 전달
  - 채팅방 기능 구현
  - 실시간 통신

### 2. UDP 통신 (udp/)
- `udp_echo_server.c`, `udp_ehco_client.c`: UDP 에코 서버/클라이언트
  - UDP 소켓 생성 및 사용
  - sendto()/recvfrom() 함수 사용
  - 비연결형 통신

### 3. 웹 서버 (webserver/)
- `pthread_basic_webserver.c`: 기본 멀티스레드 웹 서버
  - HTTP 프로토콜 처리
    - GET, POST, HEAD 메서드 지원
    - HTTP/1.1 프로토콜 구현
    - 요청 헤더 파싱 및 처리
  - 파일 서빙 기능
    - 정적 파일 서빙 (HTML, CSS, JS, 이미지 등)
    - MIME 타입 자동 감지
    - 파일 권한 검사
  - 멀티스레드 클라이언트 처리
    - pthread를 사용한 동시 접속 처리
    - 스레드 풀 구현
    - 클라이언트 연결 관리

- `pthread_log_webserver.c`: 로깅 기능이 있는 웹 서버
  - 접속 로그 기록
    - 클라이언트 IP 주소
    - 접속 시간
    - 요청 URL
    - HTTP 상태 코드
  - 파일 접근 로깅
    - 파일 경로
    - 접근 시간
    - 파일 크기
    - 전송 시간
  - 에러 처리
    - 404 Not Found
    - 403 Forbidden
    - 500 Internal Server Error
    - 에러 로그 기록

- `https_webserver.c`: HTTPS 웹 서버
  - SSL/TLS 암호화
    - OpenSSL 라이브러리 사용
    - TLS 1.2/1.3 프로토콜 지원
    - 암호화 스위트 설정
  - 인증서 관리
    - X.509 인증서 처리
    - 인증서 체인 검증
    - 인증서 갱신 처리
  - 보안 통신
    - SNI(Server Name Indication) 지원
    - 세션 재사용
    - 보안 헤더 설정

- `cppwebserver.cpp`: C++ 기반 웹 서버
  - 객체지향 설계
    - 클래스 기반 구조화
    - 상속과 다형성 활용
    - RAII 원칙 적용
  - C++ 소켓 프로그래밍
    - 소켓 클래스 래핑
    - 예외 처리
    - 스마트 포인터 사용
  - 클래스 기반 구조
    - Server 클래스
    - Client 클래스
    - Request/Response 클래스
    - Config 클래스

### 4. 데몬 프로세스 (daemon/)
- `daemon_process.c`: 기본 데몬 프로세스 구현
  - 데몬화 과정
    - fork()로 부모 프로세스와 분리
    - setsid()로 세션 리더 생성
    - umask()로 파일 권한 설정
    - chdir()로 작업 디렉토리 변경
  - 작업 디렉토리 변경
    - 루트 디렉토리(/)로 이동
    - 상대 경로 처리
    - 디렉토리 권한 검사
  - 표준 입출력 재지정
    - /dev/null로 리다이렉션
    - 로그 파일 설정
    - 에러 처리

- `daemon_server.c`: 데몬 웹 서버
  - 데몬 프로세스로 실행되는 웹 서버
    - 데몬화 및 초기화
    - 설정 파일 로드
    - 포트 바인딩
  - 멀티스레드 클라이언트 처리
    - 스레드 풀 구현
    - 동시 접속 제한
    - 타임아웃 처리
  - 로깅 및 시그널 처리
    - syslog() 통합
    - 로그 레벨 설정
    - 로그 로테이션
    - SIGTERM, SIGHUP 처리
    - 정상 종료 절차

### 5. RTSP 서버 (rtsp/)
- `rtsp_skeleton.c`: RTSP 서버 기본 구조
  - RTSP 프로토콜 구현
    - RTSP 메시지 파싱
    - 요청/응답 처리
    - 세션 관리
  - RTSP 메서드 지원
    - OPTIONS: 서버 기능 확인
    - DESCRIBE: 미디어 정보 요청
    - SETUP: 스트림 설정
    - PLAY: 재생 시작
    - PAUSE: 일시 정지
    - TEARDOWN: 세션 종료
  - 세션 관리
    - 세션 ID 생성
    - 타임아웃 처리
    - 클라이언트 상태 관리
    - 스트림 상태 추적

- `rtsp_test1.c`, `rtsp_test2.c`: RTSP 서버 테스트
  - 미디어 스트리밍
    - 비디오/오디오 스트림 처리
    - 코덱 설정
    - 버퍼 관리
  - RTP/RTCP 프로토콜
    - RTP 패킷화
    - RTCP 피드백
    - QoS 모니터링
    - 네트워크 적응
  - 실시간 비디오 전송
    - 프레임 타임스탬프
    - 동기화 처리
    - 지터 버퍼링
    - 패킷 손실 처리

### 6. 소켓 IPC (soket/)
- `socketpair.c`: 소켓 쌍을 이용한 IPC
  - socketpair() 함수 사용
    - AF_LOCAL 도메인 소켓
    - 양방향 통신 채널
    - 프로세스 간 데이터 전송
  - 부모-자식 프로세스 간 통신
    - fork()로 프로세스 생성
    - 소켓 디스크립터 상속
    - 프로세스 간 동기화
  - 양방향 데이터 전송
    - 메시지 송수신
    - 버퍼 관리
    - 에러 처리
    - 소켓 정리

## 주요 개념

### 소켓 프로그래밍
- 소켓 생성 및 초기화
  - socket(): 소켓 생성
  - bind(): 주소 바인딩
  - listen(): 연결 대기
  - accept(): 클라이언트 연결 수락

- 데이터 송수신
  - send()/recv(): TCP 데이터 송수신
  - sendto()/recvfrom(): UDP 데이터 송수신
  - 소켓 옵션 설정

- 다중 클라이언트 처리
  - fork(): 멀티프로세스
  - pthread: 멀티스레드
  - select()/poll()/epoll: I/O 다중화

### 웹 서버 구현
- HTTP 프로토콜
  - 요청/응답 처리
  - 헤더 파싱
  - 상태 코드

- 파일 서빙
  - MIME 타입 처리
  - 파일 읽기 및 전송
  - 디렉토리 리스팅

- 보안
  - SSL/TLS 암호화
  - 인증서 관리
  - HTTPS 구현

### 데몬 프로세스
- 데몬화 과정
  - fork()로 부모 프로세스와 분리
  - setsid()로 세션 리더 생성
  - 작업 디렉토리 변경
  - 파일 권한 설정

- 로깅
  - syslog() 사용
  - 로그 파일 관리
  - 로그 레벨 설정

- 시그널 처리
  - SIGTERM, SIGHUP 등 처리
  - 정상 종료 절차
  - 자원 정리

### 미디어 스트리밍
- RTSP 프로토콜
  - 세션 관리
  - 미디어 제어
  - 스트림 설정

- RTP/RTCP
  - 패킷화
  - 타임스탬프
  - QoS 관리

## 참고사항
- 대부분의 예제는 Linux 환경에서 실행되어야 합니다.
- 네트워크 예제는 방화벽 설정에 따라 차단될 수 있습니다.
- 웹 서버 예제는 포트 번호를 지정하여 실행해야 합니다.
- 데몬 프로세스는 root 권한이 필요할 수 있습니다.
- RTSP 서버는 미디어 파일이 필요할 수 있습니다. 