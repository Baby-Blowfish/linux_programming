void uploadImage(const std::string& serverAddress, int port) {
    // OpenCV로 500x600 크기의 빨간색 이미지 생성
    cv::Mat redImage(300, 400, CV_8UC3, cv::Scalar(0, 0, 255));  // 빨간색 (BGR)
    std::vector<uchar> buf;
    cv::imencode(".jpg", redImage, buf);
    std::string base64Image = base64_encode(buf.data(), buf.size());
    // JSON 데이터 생성
    json requestData = {
        {"GateNum", 1},
        {"PlateNum", "ABC123"},
        {"time", "2024-12-06T12:34:56"},
        {"imageRaw", base64Image},
        {"imageWidth", redImage.cols},
        {"imageHeight", redImage.rows}
    };
    // JSON 문자열로 변환
    std::string jsonData = requestData.dump();
    // OpenSSL 초기화
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return;
    }
    // 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        SSL_CTX_free(ctx);
        return;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(serverAddress.c_str());
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection failed!" << std::endl;
        close(sock);
        SSL_CTX_free(ctx);
        return;
    }
    // SSL 연결 설정
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL connection failed!" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        return;
    }
    // HTTP 요청 헤더 생성
    std::stringstream requestStream;
    requestStream << "POST /upload HTTP/1.1\r\n";
    requestStream << "Host: " << serverAddress << "\r\n";
    requestStream << "Content-Type: application/json\r\n";
    requestStream << "Content-Length: " << jsonData.size() << "\r\n";
    requestStream << "\r\n";
    requestStream << jsonData;
    // HTTP 요청 전송
    std::string request = requestStream.str();
    SSL_write(ssl, request.c_str(), request.size());
    // 서버 응답 받기
    char buffer[1024];
    int bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        std::cerr << "Error reading response!" << std::endl;
        ERR_print_errors_fp(stderr);
    } else {
        buffer[bytesRead] = '\0';  // null terminate the string
        std::cout << "Server Response: " << buffer << std::endl;
    }
    // 연결 종료
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
}