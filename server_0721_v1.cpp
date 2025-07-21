#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600  // Windows Vista 이상 타겟팅

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mysql.h>      // MariaDB/MySQL C API 헤더
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>  // https://github.com/nlohmann/json

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")

using namespace std;
using json = nlohmann::json;

static const int PORT = 10000;
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
std::mutex cout_mutex;

//std::unordered_map<std::string, SOCKET> clients;
const std::string targetIP = "127.0.0.1";  // 이 IP에만 메시지를 보냄
const string userIP = "10.10.20.104";
std::unordered_map<std::string, std::vector<SOCKET>> clientsList;

std::vector<std::string> word_targets = {
    "내과",
    "신경과",
    "정신건강의학과",
    "재활의학과",
    "가정의학과",
    "피부과",
    "마취통증의학과",
    "외과",
    "일반외과",
    "대장항문외과",
    "유방외과",
    "신경외과",
    "정형외과",
    "흉부외과",
    "성형외과",
    "비뇨기과",
    "산부인과",
    "소아청소년과",
    "안과",
    "이비인후과"
};

// 병원 결과 저장 Json
json result_hospital = json::array();

//struct HospitalInfo {
//    std::string hop_name;
//    std::string treat_code;
//    int num_spc;
//    std::string hop_addr;
//    std::string hop_pnum;
//    double x_cdn;  // 좌표값
//    double y_cdn;
//};



// 콘솔을 UTF-8 모드로 설정
void init_console_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
}

// UTF-8 → UTF-16 변환 및 출력
void PrintUtf8AsUnicode(const char* utf8Str) {
    std::wstring wstr = converter.from_bytes(utf8Str);
    std::wcout << wstr << L"\n";
}

// JSON 응답 전송 헬퍼
void sendJsonResponse(SOCKET client, const json& resp) {
    string out = resp.dump() + "\n";
    const char* p = out.c_str();
    int total = (int)out.size(), sent = 0;
    while (sent < total) {
        int r = send(client, p + sent, total - sent, 0);
        if (r == SOCKET_ERROR) {
            std::wcerr << L"send 실패\n";
            break;
        }
        sent += r;
    }
}

// 문자열 이스케이프
string escapeString(MYSQL* conn, const string& str) {
    char* buf = new char[str.size() * 2 + 1];
    unsigned long len = mysql_real_escape_string(conn, buf, str.c_str(), (unsigned long)str.size());
    string ret(buf, len);
    delete[] buf;
    return ret;
}

// 벡터 join
string join(const vector<string>& vec, const char* delim) {
    ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i) oss << delim;
        oss << vec[i];
    }
    return oss.str();
}

wstring Utf8ToWstring(const char* utf8Str) {
    return converter.from_bytes(utf8Str);
}

// 클라이언트 처리 함수
void handleClient(SOCKET clientSocket, MYSQL* conn) {
    MYSQL_ROW row;
    // 클라이언트 IP 얻기
    sockaddr_in addr{};
    int addrlen = sizeof(addr);
    getpeername(clientSocket, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    // InetNtopA로 IPv4 문자열 변환
    char ipStr[INET_ADDRSTRLEN] = {};
    InetNtopA(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
    wprintf(L"클라이언트 접속됨: %hs\n", ipStr);

    string clientIP(ipStr);
    // 2) 맵에 저장
    /*clients[clientIP] = clientSocket;*/
    
    std::wstring wClientIP = converter.from_bytes(clientIP);

    int clientPort = ntohs(addr.sin_port);

    // 2. 키 생성
    std::string key = clientIP + ":" + std::to_string(clientPort);

    // 3. 맵에 저장
    clientsList[clientIP].push_back(clientSocket);
    std::wcout << L"[접속] 클라이언트 " << wClientIP << L" 연결됨\n";


    char buffer[4096];
    int recvBytes;
    while ((recvBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[recvBytes] = '\0';
        PrintUtf8AsUnicode(buffer);

        json request, response;
        try {
            request = json::parse(buffer);
            string signal = request.value("signal", "");

            if (signal == "join")
            {
                 
                wprintf(L"here join\n");
                 
                string id = request.value("id", "");
                string pw = request.value("pw", "");
                string name = request.value("name", std::string{});
                string addr = request.value("adress", std::string{});
                string phone = request.value("phone", "");
                string birth = request.value("year", "");
                string sex = request.value("gender", std::string{});
                wstring addrs = converter.from_bytes(addr);
                                     
                string gender;
                 
                if (sex == "여자")
                {
                    gender = "F";
                }
                else
                {
                    gender = "M";
                }
                 
                wprintf(L"here join: %ls\n", addrs.c_str());
                                    
                string join_query = "INSERT INTO user_info(USER_ID, USER_PW, USER_NAME, USER_ADDR, USER_PNUM, USER_BIRTH, USER_SEX)"
                    "VALUES('"+id+"','"+pw+"','"+name+"','" + addr + "','" + phone + "','" + birth + "','" + gender + "')";
                 
                if (mysql_query(conn, join_query.c_str()) == 0) {
                    response["signal"] = "join_success";
                    response["result"] = "0";
                    sendJsonResponse(clientSocket, response);
                }
                else
                {
                    wprintf(L"Query Error");
                }
                     
            }
            else if (signal == "login") 
            {
                string id = request.value("id", "");
                string pw = request.value("pw", "");
                string q = "SELECT * FROM user_info WHERE USER_ID='" + escapeString(conn, id) +
                    "' AND USER_PW='" + escapeString(conn, pw) + "'";
                if (mysql_query(conn, q.c_str()) == 0) {
                    MYSQL_RES* result = mysql_store_result(conn);
                    if (!result || mysql_num_rows(result) == 0) {
                        response["signal"] = "login_result";
                        response["result"] = "1";  // 로그인 실패
                        sendJsonResponse(clientSocket, response);
                        if (result) mysql_free_result(result);
                        continue;
                    }
                    sendJsonResponse(clientSocket, json{ {"signal","login_result"},{"result","0"} });
                    mysql_free_result(result);
                }
            }
            else if (signal == "record")
            {
                string question = request.value("say_text", "");
                
                std::wstring ques = converter.from_bytes(question);
                
                wprintf(L"질문: %ls\n", ques.c_str());
                
                auto it = clientsList.find(targetIP);
                if (it != clientsList.end() && !it->second.empty()) {
                    // 예: 가장 먼저 connect 된 소켓에만 한 번 전송
                    SOCKET sock = it->second.front();

                    response["prompt"] = question;
                    sendJsonResponse(sock, response);
                }
                else {
                    std::cout << "[알림] 해당 IP의 소켓 리스트가 없습니다: "
                        << targetIP << "\n";
                }

                /*response["signal"] = "request";
                response["result"] = converter.to_bytes(L"뭐");
                sendJsonResponse(clientSocket, response);*/
            }
            else if (signal == "ai_answer")
            {
                string answer = request.value("result", "");
                wstring ai_answer = converter.from_bytes(answer);

                wprintf(L"답변 : %ls\n", ai_answer.c_str());

                std::string change_answer = converter.to_bytes(ai_answer);

                response["signal"] = "request";
                response["result"] = change_answer;

                std::vector<std::string> matched_words;

                for (const auto& word : word_targets) {
                    if (change_answer.find(word) != std::string::npos) {
                        matched_words.push_back(word);
                    }
                }
                
                auto it = clientsList.find(userIP);
                if (it != clientsList.end() && !it->second.empty()) {
                    // 예: 가장 먼저 connect 된 소켓에만 한 번 전송

                    if (!matched_words.empty())
                    {
                        std::string match_query =
                            "SELECT hi.HOP_NAME, hd.TREAT_CODE, hd.NUM_SPC, hi.HOP_ADDR, hi.HOP_PNUM, hi.X_CND, hi.Y_CND "
                            "FROM hospital_detail hd "
                            "JOIN hospital_info hi ON hd.HOP_ID = hi.HOP_ID "
                            "WHERE hd.TREAT_CODE IN (";

                        for (size_t i = 0; i < matched_words.size(); ++i) {
                            match_query += "'" + matched_words[i] + "'";
                            if (i != matched_words.size() - 1)
                                match_query += ", ";
                        }
                        match_query += ");";

                        mysql_query(conn, match_query.c_str());
                        MYSQL_RES* res = mysql_store_result(conn);
                        int num_fields = mysql_num_fields(res);
                        while ((row = mysql_fetch_row(res))) {
                            result_hospital.push_back({
                                {"hop_name", row[0] ? row[0] : ""},
                                {"treat_code", row[1] ? row[1] : ""},
                                {"num_spc", row[2] ? std::stoi(row[2]) : 0 },
                                {"hop_addr", row[3] ? row[3] : "" },
                                {"hop_pnum", row[4] ? row[4] : ""},
                                {"x_cdn", row[5] ? std::stod(row[5]) : 0.0},
                                {"y_cdn", row[6] ? std::stod(row[6]) : 0.0}
                                });
                        }

                    }

                    SOCKET sock = it->second.front();

                    sendJsonResponse(sock, response);
                    change_answer.clear();
                }
                else {
                    std::cout << "[알림] 해당 IP의 소켓 리스트가 없습니다: "<< targetIP << "\n";
                }
            }
            else {
                response["status"] = "error";
                response["error"] = "Invalid signal";
                sendJsonResponse(clientSocket, response);
            }
        }
        catch (json::exception& e) {
            sendJsonResponse(clientSocket,
                json{ {"status","error"},{"error",string("JSON parse error: ") + e.what()} });
        }
    }

    closesocket(clientSocket);
    std::wcout << L"클라이언트 연결 종료\n";
}

int main() {
    init_console_utf8();
    std::wcout << L"=== 프로그램 진입 ===\n";

    // 1) Winsock 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[ERROR] WSAStartup 실패\n";
        return 1;
    }

    // MariaDB 연결
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        std::wcerr << L"mysql_init 실패\n";
        return 1;
    }
    if (!mysql_real_connect(conn,
        "10.10.20.105",
        "USER",
        "1234",
        "apayo",
        3306, nullptr, 0)) {
        std::wcerr << L"DB 연결 실패: " << converter.from_bytes(mysql_error(conn)) << L"\n";
        mysql_close(conn);
        return 1;
    }
    std::wcout << L"MariaDB 연결 성공\n";

    // WinSock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::wcerr << L"WSAStartup 실패\n";
        mysql_close(conn);
        return 1;
    }

    // 서버 소켓 생성
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::wcerr << L"소켓 생성 실패\n";
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (::bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::wcerr << L"바인드 실패\n";
        closesocket(serverSocket);
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::wcerr << L"리스닝 실패\n";
        closesocket(serverSocket);
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    std::wcout << L"서버 대기중... 포트 " << PORT << L"\n";

    // 다중 클라이언트 처리
    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = ::accept(
            serverSocket,
            reinterpret_cast<SOCKADDR*>(&clientAddr),
            &clientAddrSize
        );
        if (clientSocket == INVALID_SOCKET) {
            std::wcerr << L"클라이언트 연결 실패\n";
            continue;
        }
        // 스레드로 분기 처리
        std::thread t(handleClient, clientSocket, conn);
        t.detach();
    }

    // 이 코드는 도달하지 않음
    closesocket(serverSocket);
    WSACleanup();
    mysql_close(conn);


    return 0;
}


