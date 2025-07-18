//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <iostream>
//#include <fcntl.h>
//#include <io.h>
//
//#pragma comment(lib, "ws2_32.lib")
//
//int PORT = 10000;
//
//// UTF-8 문자열을 UTF-16으로 변환해서 출력하는 함수
//void PrintUtf8AsUnicode(const char* utf8Str) {
//    // UTF-8 -> UTF-16 변환 길이 계산
//    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
//    if (len > 0) {
//        wchar_t* wstr = new wchar_t[len];
//        MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wstr, len);
//
//        std::wcout << L"클라이언트 메시지: " << wstr << L"\n";
//
//        delete[] wstr;
//    }
//}
//
//int main() {
//    // 콘솔 출력을 UTF-16 모드로 변경
//    _setmode(_fileno(stdout), _O_U16TEXT);
//
//    WSADATA wsaData;
//    SOCKET serverSocket, clientSocket;
//    sockaddr_in serverAddr, clientAddr;
//    int clientAddrSize = sizeof(clientAddr);
//
//    // WinSock 초기화
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::wcerr << L"WSAStartup 실패\n";
//        return 1;
//    }
//
//    // 소켓 생성
//    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//    if (serverSocket == INVALID_SOCKET) {
//        std::wcerr << L"소켓 생성 실패\n";
//        WSACleanup();
//        return 1;
//    }
//
//    // 서버 주소 설정
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = INADDR_ANY;
//    serverAddr.sin_port = htons(PORT);
//
//    // 바인드
//    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::wcerr << L"바인드 실패\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    // 리슨
//    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
//        std::wcerr << L"리스닝 실패\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        return 1;
//    }
//
//    wchar_t buf[100];
//    // "%d" 자리에 PORT 값을 넣고, wide 문자열을 buf에 작성
//    swprintf_s(buf, L"서버 대기중... 포트 %d\n", PORT);
//    std::wcout << buf;
//    /*std::wcout << L"서버 대기중... 포트 {}\n", PORT;*/
//
//    while (true) {
//        clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
//        if (clientSocket == INVALID_SOCKET) {
//            std::wcerr << L"클라이언트 연결 실패\n";
//            continue;
//        }
//
//        std::wcout << L"클라이언트 접속됨!\n";
//
//        char buffer[1024];
//        int recvBytes;
//
//        // 클라이언트와 계속 통신
//        while ((recvBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
//            buffer[recvBytes] = '\0'; // 문자열 끝 처리
//            PrintUtf8AsUnicode(buffer);
//        }
//
//        if (recvBytes == 0) {
//            std::wcout << L"클라이언트 정상 종료\n";
//        }
//        else {
//            std::wcerr << L"recv 에러\n";
//        }
//
//        closesocket(clientSocket);
//        std::wcout << L"클라이언트 접속 종료\n";
//    }
//
//    closesocket(serverSocket);
//    WSACleanup();
//    return 0;
//}