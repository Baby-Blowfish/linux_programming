# Linux Programming Examples

이 프로젝트는 Linux 시스템 프로그래밍의 기본적인 예제들을 포함하고 있습니다.

## 개별 파일 설명

### 기본 입출력 및 시스템 콜
- `error.c`: 기본적인 write() 시스템 콜을 사용한 "Hello World" 출력 예제
- `exit.c`: 프로그램 종료를 위한 exit() 함수 사용 예제
- `f.c`: 표준 입출력 파일 디스크립터(fd) 사용 예제
  - fd 0 (stdin): scanf() 사용
  - fd 1 (stdout): printf() 사용
  - fd 2 (stderr): perror() 사용

### 파일 시스템 관련
- `list.c`: 디렉토리 리스팅 프로그램
  - 파일 권한, 소유자, 그룹, 크기, 수정 시간 등 상세 정보 출력
  - 재귀적으로 하위 디렉토리 탐색
  - opendir(), readdir(), stat() 등의 시스템 콜 사용
- `chmod.c`: 파일 권한 변경 예제
  - chmod() 시스템 콜을 사용한 파일 권한 수정
  - 사용자, 그룹, 기타 사용자에 대한 권한 설정

### 메모리 및 문자열
- `size.c`: sizeof와 strlen의 차이점을 보여주는 예제
  - 배열과 포인터의 크기 차이
  - 문자열 길이 계산

### 시간 관련
- `time.c`: 다양한 시간 관련 함수 사용 예제
  - time(): 현재 시간 (초 단위)
  - gettimeofday(): 마이크로초 단위 시간
  - ctime(): 사람이 읽기 쉬운 시간 문자열
  - localtime(): 지역 시간 변환
  - strftime(): 시간 포맷팅

### 동시성 및 원자성
- `atomic.cpp`: C++ atomic 타입 사용 예제
  - fetch_add(): 원자적 덧셈
  - fetch_sub(): 원자적 뺄셈
- `chrno.cpp`: C++ chrono 라이브러리 사용 예제
  - system_clock을 사용한 시간 측정
  - 나노초 단위 정밀도

### 환경 변수
- `env.c`: 환경 변수 출력 예제
  - extern char **environ을 사용한 환경 변수 접근

### 저수준 파일 I/O (low_level_file_IO/)
- `copy.c`: 파일 복사 예제
  - open(), read(), write() 시스템 콜을 사용한 파일 복사
- `seek_write.c`: 파일 위치 제어 예제
  - lseek()를 사용한 파일 내 위치 이동 및 쓰기
- `write_fsync.c`: 파일 동기화 예제
  - fsync()를 사용한 디스크 동기화
- `write_only.c`: 파일 쓰기 예제
  - 기본적인 파일 쓰기 작업

### 표준 파일 I/O (std_file_IO/)
- `hitkey.c`, `hitkey_ascii.c`, `hitkey_combin.c`: 키보드 입력 처리 예제
  - 다양한 키보드 입력 처리 방식
  - ASCII 코드 및 조합키 처리
- `buffer_mode_test.c`: 버퍼 모드 테스트
  - 다양한 버퍼링 모드 실험
- `flush_line_test.c`: 라인 버퍼링 테스트
  - 라인 단위 버퍼 플러시
- `logger.c`: 로깅 시스템 예제
  - 파일 기반 로깅 시스템 구현
- `position_control.c`: 파일 위치 제어
  - fseek()를 사용한 파일 위치 제어
- `stdin_buffer_clean.c`: 표준 입력 버퍼 정리
  - 입력 버퍼 관리 방법
- `full_file_io.c`: 완전한 파일 I/O 예제
  - fopen(), fread(), fwrite() 등의 표준 I/O 함수 사용

## 참고사항
- 대부분의 예제는 Linux 환경에서 실행되어야 합니다.
- 일부 예제는 root 권한이 필요할 수 있습니다.
- 파일 시스템 관련 예제는 적절한 권한이 있는 디렉토리에서 실행해야 합니다.
