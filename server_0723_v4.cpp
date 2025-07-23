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
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // https://github.com/nlohmann/json
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#pragma execution_character_set("utf-8")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")

using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;
static const int PORT = 10005;
//static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
std::mutex cout_mutex;
std::mutex clientsListMutex;

//std::unordered_map<std::string, SOCKET> clients;
const std::string targetIP = "127.0.0.1";  // 이 IP에만 메시지를 보냄
const string userIP = "10.10.20.104";
std::unordered_map<std::string, std::vector<SOCKET>> clientsList;

std::vector<std::wstring> word_targets = {L"내과",L"신경과",L"정신건강의학과",L"재활의학과",L"가정의학과",L"피부과",L"마취통증의학과",L"외과",L"일반외과",L"대장항문외과",L"유방외과",L"신경외과",L"정형외과",L"흉부외과",L"성형외과",L"비뇨기과",L"산부인과",L"소아청소년과",L"안과",L"이비인후과"};
vector<std::wstring> matched_words;
// 병원 결과 저장 Json
json result_hospital = json::array();

struct user_infos
{
    string user_id;
    string user_name;
    string user_addr;
};

vector<user_infos> all_users;

std::wstring Utf8ToUtf16(const std::string& utf8) {
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);


    if (size <= 0) return L"[변환 실패]";
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    result.pop_back();  // 널 문자 제거
    return result;
}

inline std::string Utf16ToUtf8(const std::wstring& utf16)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, utf16.data(), (int)utf16.size(), NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.data(), (int)utf16.size(), &result[0], size, NULL, NULL);
    return result;
}


// 콘솔을 UTF-8 모드로 설정
void init_console_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
}

// UTF-8 → UTF-16 변환 및 출력
void PrintUtf8AsUnicode(const char* utf8Str) {
    std::wstring wstr = Utf8ToUtf16(utf8Str);
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
    return Utf8ToUtf16(utf8Str);
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf;
    localtime_s(&tm_buf, &t);  // 안전한 함수 사용

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

// 사용자 JSON 파일에 데이터 추가 (없으면 생성)
void saveUserDataToJson(const std::string& userId, const std::string& userName, const json& dataToStore) {
    if (userId.empty() || userName.empty()) return;

    std::string basePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + userId;
    std::filesystem::create_directories(basePath);

    std::filesystem::path filePath = std::filesystem::path(basePath) / (userName + ".json");
    json fileContent;

    if (std::filesystem::exists(filePath)) {
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            try {
                inFile >> fileContent;
            }
            catch (const std::exception& e) {
                std::wcerr << L"[오류] 기존 JSON 파싱 실패: " << Utf8ToUtf16(e.what()) << std::endl;
                fileContent = json::array();
            }
            inFile.close();
        }
        else {
            std::wcerr << L"[경고] JSON 파일 열기 실패\n";
            fileContent = json::array();
        }
    }
    else {
        fileContent = json::array();
    }

    if (!fileContent.is_array()) {
        std::wcerr << L"[경고] 기존 데이터가 배열이 아님. 초기화.\n";
        fileContent = json::array();
    }

    json newEntry;
    newEntry["timestamp"] = getCurrentTimestamp();
    newEntry["data"] = dataToStore;

    fileContent.insert(fileContent.begin(), newEntry);

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        std::wcerr << L"[오류] 파일 저장 실패: " << Utf8ToUtf16(filePath.string()) << std::endl;
        return;
    }

    outFile << fileContent.dump(4);
    outFile.close();

    std::wcout << L"✅ 사용자 데이터 저장 완료: " << Utf8ToUtf16(filePath.string()) << std::endl;
}

// Json파일 내용 삭제
void deleteEntryByTimestamp(const std::string& userId, const std::string& userName, const std::string& targetTimestamp) {
    if (userId.empty() || userName.empty() || targetTimestamp.empty()) {
        std::wcerr << L"[오류] userId, userName 또는 timestamp가 비어 있습니다.\n";
        return;
    }

    std::string basePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + userId;
    std::filesystem::path filePath = std::filesystem::path(basePath) / (userName + ".json");

    if (!std::filesystem::exists(filePath)) {
        std::wcerr << L"[오류] 파일이 존재하지 않습니다: " << Utf8ToUtf16(filePath.string()) << std::endl;
        return;
    }

    json fileContent;
    std::ifstream inFile(filePath);
    try {
        inFile >> fileContent;
    }
    catch (const std::exception& e) {
        std::wcerr << L"[오류] JSON 파싱 실패: " << Utf8ToUtf16(e.what()) << std::endl;
        return;
    }
    inFile.close();

    if (!fileContent.is_array()) {
        std::wcerr << L"[오류] JSON 구조가 배열 형식이 아닙니다.\n";
        return;
    }

    bool deleted = false;
    json updatedContent = json::array();

    for (const auto& entry : fileContent) {
        if (entry.contains("timestamp") && entry["timestamp"] == targetTimestamp) {
            deleted = true;
            continue;  // 삭제 대상 건너뛰기
        }
        updatedContent.push_back(entry);
    }

    if (!deleted) {
        std::wcout << L"[알림] '" << Utf8ToUtf16(targetTimestamp) << L"' 타임스탬프 항목을 찾을 수 없습니다.\n";
        return;
    }

    std::ofstream outFile(filePath);
    outFile << updatedContent.dump(4);
    outFile.close();

    std::wcout << L"✅ 타임스탬프 항목 삭제 완료: " << Utf8ToUtf16(filePath.string()) << std::endl;
}

// 날짜 형식 검증 함수
bool isValidDateFormat(const std::string& s) {
    std::tm t = {};
    std::istringstream ss(s);
    ss >> std::get_time(&t, "%Y-%m-%d");
    return !ss.fail();
}

// 나이 계산 함수
int calculateAge(const std::string& birthDateStr) {
    // 1. 날짜 형식 유효성 검사
    if (!isValidDateFormat(birthDateStr)) {
        std::cerr << "⚠️ 날짜 형식 오류: [" << birthDateStr << "] (형식은 YYYY-MM-DD이어야 함)" << std::endl;
        return -1;
    }

    // 2. 생년월일 파싱
    std::tm birth = {};
    std::istringstream ss(birthDateStr);
    ss >> std::get_time(&birth, "%Y-%m-%d");

    // 3. 현재 시간 가져오기 (localtime_s 사용)
    std::time_t now = std::time(nullptr);
    std::tm current = {};
    if (localtime_s(&current, &now) != 0) {
        std::cerr << "❌ 현재 시간 조회 실패" << std::endl;
        return -1;
    }

    // 4. 나이 계산
    int age = (current.tm_year + 1900) - (birth.tm_year + 1900);
    if ((current.tm_mon < birth.tm_mon) ||
        (current.tm_mon == birth.tm_mon && current.tm_mday < birth.tm_mday)) {
        age--; // 생일 안 지났으면 1살 차감
    }

    // 5. 음수 나이 예외 처리
    if (age < 0 || age > 150) {
        std::cerr << "❌ 비정상적인 나이 계산 결과: " << age << " (입력: " << birthDateStr << ")" << std::endl;
        return -1;
    }

    return age;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// 주소에 대한 좌표값 계산
std::pair<double, double> getCoordinatesFromAddress(const std::string& address) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "CURL 초기화 실패\n";
        return { -1.0, -1.0 };
    }

    // ✅ 주소 인코딩
    char* encoded = curl_easy_escape(curl, address.c_str(), static_cast<int>(address.length()));
    if (!encoded) {
        std::cerr << "주소 인코딩 실패\n";
        curl_easy_cleanup(curl);
        return { -1.0, -1.0 };
    }

    std::string url = "https://dapi.kakao.com/v2/local/search/address.json?query=" + std::string(encoded);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Authorization: KakaoAK 3e79cee032866364a65db09ff4d13f10"); // ✅ KakaoAK 접두사 주의

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);

    // 해제
    curl_free(encoded);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "요청 실패: " << curl_easy_strerror(res) << std::endl;
        return { -1.0, -1.0 };
    }

    // JSON 파싱
    try {
        json response = json::parse(readBuffer);
        if (!response["documents"].empty()) {
            std::string x_str = response["documents"][0]["x"];
            std::string y_str = response["documents"][0]["y"];
            double x = std::stod(x_str);
            double y = std::stod(y_str);
            return { x, y }; // (경도, 위도)
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
    }

    return { -1.0, -1.0 }; // 실패 시
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
    
    std::wstring wClientIP = Utf8ToUtf16(clientIP);

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
                wstring addrs = Utf8ToUtf16(addr);
                                     
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
                    wstring basePath = L"C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\";
                    wstring userFolder = Utf8ToUtf16(id);  // id는 string이므로 변환 필요
                    wstring fullPath = basePath + userFolder;

                    if (!CreateDirectoryW(fullPath.c_str(), NULL)) {
                        DWORD err = GetLastError();
                        if (err != ERROR_ALREADY_EXISTS) {
                            wprintf(L"Failed to create directory. Error code: %d\n", err);
                            // 필요시 폴더 생성 실패 처리 로직 추가
                        }
                    }

                    string query = "SELECT U_INFO_ID FROM user_info WHERE USER_ID = '" + id + "'";
                    mysql_query(conn, query.c_str());
                    MYSQL_RES* res = mysql_store_result(conn);
                    row = mysql_fetch_row(res);

                    string u_info_id = row[0] ? row[0] : "";
                    wprintf(L"여기 아이디값:", u_info_id.c_str());

                    mysql_free_result(res);

                    std::string mem_insert =
                        "INSERT INTO mem_info (U_INFO_ID, MEM_NAME, MEM_BIRTH, MEM_SEX) "
                        "VALUES('" + u_info_id + "','" + name + "','" + birth + "','" + gender + "')";

                    if (mysql_query(conn, mem_insert.c_str()) == 0) {

                        response["signal"] = "join_success";
                        response["result"] = "0";
                        sendJsonResponse(clientSocket, response);
                        
                    }
                    else
                    {
                        response["signal"] = "join_success";
                        response["result"] = "1";
                        sendJsonResponse(clientSocket, response);
                    }
                }
                else
                {
                    wprintf(L"Query Error");
                }
                     
            }
            else if (signal == "login") 
            {
                
                int age = 0;
                string id = request.value("id", "");
                string pw = request.value("pw", "");
                string q = "SELECT * FROM user_info WHERE USER_ID='" + escapeString(conn, id) +
                    "' AND USER_PW='" + escapeString(conn, pw) + "'";

                if (mysql_query(conn, q.c_str()) == 0) 
                {
                    MYSQL_RES* result = mysql_store_result(conn);
                    if (!result || mysql_num_rows(result) == 0) {
                        // 로그인 실패 처리
                        response["signal"] = "login_result";
                        response["result"] = "1";
                        sendJsonResponse(clientSocket, response);
                        if (result) mysql_free_result(result);
                        continue;
                    }
                    wprintf(L"여기요!\n");
                    row = mysql_fetch_row(result);
                    string u_info_id = "0";  // 기본값
                    
                    user_infos userinfo;
                    if (row && row[0]) {
                        u_info_id = row[0];
                    }

                    if (row) {
                        if (row[1]) {
                            userinfo.user_id.append(row[1]);
                        }

                        if (row[3]) {
                            userinfo.user_name.append(row[3]);
                        }

                        if (row[4])
                        {
                            userinfo.user_addr.append(row[4]);
                        }
                    }
                    

                    string query = "SELECT * FROM mem_info WHERE U_INFO_ID = '" + u_info_id + "'";
                    mysql_query(conn, query.c_str());
                    MYSQL_RES* res = mysql_store_result(conn);
                    wprintf(L"여기요!!\n");
                    if (!res)
                    {
                        sendJsonResponse(clientSocket, json{ {"signal","login_result"},{"result","0"} });
                    }
                    else
                    {
                        json member = json::array();

                        while ((row = mysql_fetch_row(res))) {
                            string name = row[2] ? row[2] : "";
                            string birth = row[3] ? row[3] : "";
                            string sex = row[4] ? row[4] : "";

                            string gender = (sex == "M") ? "남자" : "여자";

                            member.push_back(json{
                                {"mem_name", name},
                                {"birth", birth},
                                {"gender", gender}
                                });
                        }
                        wprintf(L"여기요!!!!!\n");
                        response["signal"] = "login_result";
                        response["result"] = "0";
                        response["profile"] = member;
                        wprintf(L"여기요!!!!!!\n");

                        std::wstring wjson = Utf8ToUtf16(response.dump(4));  // 보기 좋게 들여쓰기
                        wprintf(L"제이슨!%ls\n", wjson.c_str());

                        sendJsonResponse(clientSocket, response);
                        
                    }
                    
                    mysql_free_result(res);

                }
                
            }
            else if (signal == "profile_create")
            {
                std::string id = request.value("id", "");
                std::string mem_name = request.value("nickname", "");
                std::string mem_brith = request.value("birth", "");
                std::string mem_sex = request.value("gender", "");

                // SQL Injection 방지 처리
                std::string safe_id = escapeString(conn, id);
                std::string safe_name = escapeString(conn, mem_name);
                std::string safe_birth = escapeString(conn, mem_brith);
                std::string safe_sex = escapeString(conn, mem_sex);

                std::string id_search = "SELECT U_INFO_ID FROM user_info WHERE USER_ID ='" + safe_id + "'";
                if (mysql_query(conn, id_search.c_str()) != 0) {
                    std::cerr << "MySQL query failed: " << mysql_error(conn) << std::endl;
                    return;
                }

                MYSQL_RES* res = mysql_store_result(conn);
                std::string u_info_id = "0"; // 기본값

                if (res != nullptr) {
                    MYSQL_ROW row = mysql_fetch_row(res);
                    if (row != nullptr && row[0] != nullptr) {
                        u_info_id = std::string(row[0]);
                    }
                    mysql_free_result(res);
                }

                // 성별 변환
                std::string gender = "";
                if (mem_sex == "여자" || mem_sex == "female" || mem_sex == "F") {
                    gender = "F";
                }
                else
                {
                    gender = "M";
                }

                std::string mem_insert =
                    "INSERT INTO mem_info (U_INFO_ID, MEM_NAME, MEM_BIRTH, MEM_SEX) "
                    "VALUES('" + u_info_id + "','" + safe_name + "','" + safe_birth + "','" + gender + "')";

                if (mysql_query(conn, mem_insert.c_str()) == 0) {

                    string query = "SELECT * FROM mem_info WHERE U_INFO_ID = '" + u_info_id + "'";
                    mysql_query(conn, query.c_str());
                    MYSQL_RES* res = mysql_store_result(conn);
                    int age = 0;
                    if (!res)
                    {
                        sendJsonResponse(clientSocket, json{ {"signal","profile_success"},{"result","0"} });
                    }
                    else
                    {
                        json member = json::array();
                        while ((row = mysql_fetch_row(res)))
                        {
                            string sex = row[4] ? row[4] : "";
                            string gender = "";
                            if (sex == "M")
                            {
                                gender = "남자";
                            }
                            else
                            {
                                gender = "여자";
                            }
                            member.push_back({
                                {"mem_name", row[2] ? row[2] : ""},
                                {"mem_birth", row[3] ? row[3] : ""},
                                {"mem_sex", gender}
                                });
                        }

                        response["signal"] = "profile_success";
                        response["result"] = "0";
                        response["profile"] = member;

                        sendJsonResponse(clientSocket, response);
                    }
                    mysql_free_result(res);
                }
                else {
                    std::cerr << "INSERT 쿼리 실패: " << mysql_error(conn) << std::endl;
                    json response = {
                        { "signal", "profile_fail" },
                        { "result", "1" }
                    };
                    sendJsonResponse(clientSocket, response);
                }
            }
            else if (signal == "record")
            {
                string question = request.value("say_text", "");
                
                std::wstring ques = Utf8ToUtf16(question);
                
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
                std::wstring w_signal = Utf8ToUtf16(signal);
                std::wcout << L"[디버그] 수신된 signal 값: " << w_signal << std::endl;

                string answer = request.value("result", "");
                wstring ai_answer = Utf8ToUtf16(answer);

                /*wprintf(L"답변 : %ls\n", ai_answer.c_str());*/

                std::wstring change_answer = Utf8ToUtf16(answer);

                response["signal"] = "request";
                response["result"] = answer;

                /*vector<std::wstring> matched_words;*/

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
                            std::string word_utf8 = Utf16ToUtf8(matched_words[i]); // 변환 함수 사용
                            match_query += "'" + word_utf8 + "'";
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
            else if (signal == "save_chat")
            {
                string user_id = request.value("id", "");
                string user_name = request.value("name", ""); // 클라이언트에서 name도 함께 보내야 함
                json messagesJson = json::array(); // 기본값

                if (request.contains("messages")) {
                    try {
                        if (request["messages"].is_array()) {
                            messagesJson = request["messages"];
                        }
                        else {
                            std::wcerr << L"⚠️ 'messages' 키는 존재하지만 배열이 아님" << std::endl;
                        }
                    }
                    catch (const std::exception& e) {
                        std::wcerr << L"❌ 'messages' 파싱 예외: " << Utf8ToUtf16(e.what()) << std::endl;
                    }
                }
                else {
                    std::wcerr << L"⚠️ 'messages' 키가 존재하지 않음" << std::endl;
                }

                saveUserDataToJson(user_id, user_name, messagesJson);

                response["signal"] = "save_chat_result";
                response["result"] = "0";

                sendJsonResponse(clientSocket, response);
            }
            else if (signal == "record_delete")
            {
                string user_id = request.value("id", "");
                string user_name = request.value("name", "");
                string timestamp = request.value("time", "");

                deleteEntryByTimestamp(user_id, user_name, timestamp);

                response["signal"] = "delete_result";
                response["result"] = "0";

                sendJsonResponse(clientSocket, response);

            }
            else if (signal == "reload_chat")
            {
                string user_id = request.value("id", "");
                string user_name = request.value("name", "");

                // 기본 응답 구조
                response["signal"] = "reload_chat_result";
                response["data"] = json::array();  // 채팅 목록 배열로 초기화

                try {
                    // 경로 설정
                    std::string filePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + user_id + "\\" + user_name + ".json";

                    // 파일 존재 확인
                    if (!std::filesystem::exists(filePath)) {
                        sendJsonResponse(clientSocket, response);
                        return;
                    }

                    // 파일 열기 및 읽기
                    std::ifstream inFile(filePath);
                    json fileContent;
                    inFile >> fileContent;

                    // 유효한 JSON 배열인지 확인
                    if (!fileContent.is_array()) {
                        sendJsonResponse(clientSocket, response);
                        return;
                    }

                    // 각 항목에 대해 timestamp와 data를 response에 추가
                    for (const auto& entry : fileContent) {
                        if (entry.contains("timestamp") && entry.contains("data")) {
                            json record;
                            record["timestamp"] = entry["timestamp"];
                            record["data"] = entry["data"];
                            response["data"].push_back(record);
                        }
                    }

                    // 응답 전송
                    sendJsonResponse(clientSocket, response);
                }
                catch (const std::exception& e) {
                    response["error"] = "데이터 처리 중 오류 발생";
                    sendJsonResponse(clientSocket, response);
                }
            }
            else if (signal == "chat_detail_request")
            {
                std::string user_id = request.value("id", "");
                std::string user_name = request.value("name", "");
                std::string timestamp = request.value("timestamp", "");

                json response;
                response["signal"] = "chat_detail_response";
                response["messages"] = json::array();  // 항상 배열로 초기화

                try {
                    std::string filePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + user_id + "\\" + user_name + ".json";

                    if (!std::filesystem::exists(filePath)) {
                        sendJsonResponse(clientSocket, response); // 파일 없음 → 빈 배열 전송
                        return;
                    }

                    std::ifstream inFile(filePath);
                    json fileContent;
                    inFile >> fileContent;

                    if (!fileContent.is_array()) {
                        sendJsonResponse(clientSocket, response); // 잘못된 형식
                        return;
                    }

                    for (const auto& entry : fileContent) {
                        if (entry.contains("timestamp") && entry["timestamp"] == timestamp) {
                            if (entry.contains("data")) {
                                const auto& data = entry["data"];

                                // 문자열일 경우 -> 배열로 감싸서 저장
                                if (data.is_string()) {
                                    response["messages"].push_back(data.get<std::string>());
                                }
                                // 배열일 경우 -> 요소 하나씩 복사
                                else if (data.is_array()) {
                                    for (const auto& item : data) {
                                        if (item.is_string()) {
                                            response["messages"].push_back(item.get<std::string>());
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }

                    sendJsonResponse(clientSocket, response);
                }
                catch (const std::exception& e) {
                    std::cerr << "예외 발생: " << e.what() << std::endl;
                    response["error"] = "서버 오류 발생";
                    sendJsonResponse(clientSocket, response);
                }
            }
            else if (signal == "req_hop")
            {
                string target_id = request.value("id", "");
                std::string search_id = target_id;
                std::string found_addr = "";

                for (const auto& user : all_users) {
                    if (user.user_id == search_id) {
                        found_addr = user.user_addr;
                        break;
                    }
                }

                auto [x, y] = getCoordinatesFromAddress(found_addr);

                //json coordinate = json::array();

                //for (const auto& word : matched_words) {
                //    std::string utf8_word = Utf16ToUtf8(word);  // UTF-8로 변환
                //    coordinate.push_back(utf8_word);
                //}
                if (!x == 0 && !y == 0) {
                    double inputLat = y;  // 위도
                    double inputLng = x;  // 경도

                    std::string query =
                        "SELECT "
                        "h.HOP_NAME, "
                        "h.HOP_ADDR, "
                        "h.X_CDN, "
                        "h.Y_CDN, "
                        "h.TYPE_CODE, "
                        "d.TREAT_CODE, "
                        "d.NUM_SPC, "
                        "h.HOP_PNUM, "
                        "("
                        "    6371 * acos("
                        "        cos(radians(" + std::to_string(inputLat) + ")) * cos(radians(h.Y_CDN)) * "
                        "        cos(radians(h.X_CDN) - radians(" + std::to_string(inputLng) + ")) + "
                        "        sin(radians(" + std::to_string(inputLat) + ")) * sin(radians(h.Y_CDN))"
                        "    )"
                        ") AS distance_km "
                        "FROM hospital_info h "
                        "JOIN hospital_detail d ON h.HOP_ID = d.HOP_ID "
                        "HAVING distance_km <= 3 "
                        "ORDER BY distance_km ASC;";

                    // MySQL 쿼리 실행
                    json hospital_info = json::array();
                    if (mysql_query(conn, query.c_str()) == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        MYSQL_ROW row;

                        if (res) {
                            while ((row = mysql_fetch_row(res))) {
                                std::string name = row[0] ? row[0] : "";
                                std::string address = row[1] ? row[1] : "";
                                std::string x_cdn = row[2] ? row[2] : "";
                                std::string y_cdn = row[3] ? row[3] : "";
                                std::string type_code = row[4] ? row[4] : "";
                                std::string treat_code = row[5] ? row[5] : "";
                                std::string num_spc = row[6] ? row[6] : "";
                                std::string phone = row[7] ? row[7] : "";
                                std::string distance = row[8] ? row[8] : "";

                                // 하나의 병원 정보를 JSON 객체로 구성
                                json hospital = {
                                    {"name", name},
                                    {"address", address},
                                    {"x", x_cdn},
                                    {"y", y_cdn},
                                    {"type_code", type_code},
                                    {"treat_code", treat_code},
                                    {"num_spc", num_spc},
                                    {"phone", phone},
                                    {"distance_km", distance}
                                };

                                // 배열에 추가
                                hospital_info.push_back(hospital);
                            }
                            mysql_free_result(res);
                            response["signal"] = "req_hop_result";
                            response["hospital"] = hospital_info;
                            sendJsonResponse(clientSocket, response);
                        }
                        else {
                           
                            std::string errorStr = mysql_error(conn);                // 오류 메시지 (UTF-8 또는 ANSI)
                            std::wstring wideError = Utf8ToUtf16(errorStr);          // UTF-16으로 변환
                            std::wcerr << L"MySQL 결과 처리 오류: " << wideError << std::endl;
                        }


                    }
                    else {
                        std::string errStr = mysql_error(conn);
                        std::wstring wideErr = Utf8ToUtf16(errStr);  // UTF-8 → UTF-16 변환 함수 필요

                        std::wcerr << L"MySQL 쿼리 오류: " << wideErr << std::endl;
                    }
                }
                else {
                    std::wcerr << L"좌표 변환 실패: 주소가 유효하지 않거나 변환 실패" << std::endl;
                }
 
                response["signal"] = "";
                response["coordinate"] = "";
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

    if (recvBytes <= 0) {
        std::wcout << L"클라이언트 연결 종료됨\n";
        closesocket(clientSocket);

        {
            std::lock_guard<std::mutex> lock(clientsListMutex);  // 🔐 락 걸기

            auto it = std::find_if(clientsList.begin(), clientsList.end(),
                [clientSocket](const auto& pair) {
                    return std::find(pair.second.begin(), pair.second.end(), clientSocket) != pair.second.end();
                });

            if (it != clientsList.end()) {
                auto& socketList = it->second;
                socketList.erase(
                    std::remove(socketList.begin(), socketList.end(), clientSocket),
                    socketList.end()
                );

                if (socketList.empty()) {
                    clientsList.erase(it);
                }
            }
        } // 🔓 lock_guard 블록 끝나면 자동 unlock
    }
    //closesocket(clientSocket);
    //std::wcout << L"클라이언트 연결 종료\n";
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
        std::wcerr << L"DB 연결 실패: " << Utf8ToUtf16(mysql_error(conn)) << L"\n";
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


