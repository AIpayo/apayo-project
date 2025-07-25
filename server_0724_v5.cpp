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
#include <set>
#include <iomanip>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // https://github.com/nlohmann/json
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#pragma execution_character_set("utf-8")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")

using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;
static const int PORT = 10006;
//static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
std::mutex cout_mutex;
std::mutex clientsListMutex;

//std::unordered_map<std::string, SOCKET> clients;
const std::string targetIP = "127.0.0.1";  // 이 IP에만 메시지를 보냄
const string userIP = "10.10.20.105";
std::unordered_map<std::string, std::vector<SOCKET>> clientsList;
std::unordered_map<std::string, SOCKET> clientSockets;

vector<string> all_user_ip = { {"10.10.20.104"}, {"10.10.20.101"}, {"10.10.20.105"} };
vector<string> users_ip = {};

struct Message {
    std::string role;
    std::string content;
};
std::unordered_map<std::string, std::vector<Message>> user_sessions;

std::vector<std::wstring> word_targets = {L"내과",L"신경과",L"정신건강의학과",L"재활의학과",L"가정의학과",L"피부과",L"마취통증의학과",L"외과",L"일반외과",L"대장항문외과",L"유방외과",L"신경외과",L"정형외과",L"흉부외과",L"성형외과",L"비뇨기과",L"산부인과",L"소아청소년과",L"안과",L"이비인후과"};
const std::map<std::wstring, std::string> department_code_map = {
    {L"피부과", "DERM"},
    {L"내과", "IM"},
    {L"정형외과", "ORTHO"},
    {L"소아과", "PED"},
    {L"이비인후과", "ENT"},
    {L"신경과", "NEURO"},
    {L"비뇨기과", "URO"},
    {L"안과", "OPHTH"},
    {L"산부인과", "OBGYN"},
    {L"재활의학과", "REHAB"}
};

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
std::wstring department_text;
std::set<std::wstring> matched_departments;        // 중복 제거용
std::vector<std::string> matched_codes;            // DB 조회용 코드 리스트


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

// Base64 인코딩 함수
std::string Base64Encode(const std::string& input) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    int val = 0, valb = -6;

    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) result.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    return result;
}

// 유틸 함수: 한글 포함 여부 검사
bool ContainsHangul(const std::string& str) {
    std::wregex hangul(L"[가-힣]");
    std::wstring wstr = Utf8ToUtf16(str);
    return std::regex_search(wstr, hangul);
}

// 파일 이름 생성 함수
std::string GetSafeFileName(const std::string& userName) {
    if (ContainsHangul(userName)) {
        return Base64Encode(userName);  // 한글 포함 시 Base64 인코딩
    }
    else {
        return userName;  // 영문이면 그대로 사용
    }
}

// 안전한 파일 이름 생성 (한글 포함 가능)
std::string MakeSafeFilenameFromName(const std::string& userName) {
    return Base64Encode(userName); // 예: "이보훈" → "7Iqk7ZWE7JuU"
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

//// 사용자 JSON 파일에 데이터 추가 (없으면 생성)
void saveUserDataToJson(const std::string& userId, const std::string& userName, const json& dataToStore) {
    
    std::string user_name = GetSafeFileName(userName);

    if (userId.empty() || userName.empty()) return;

    std::string basePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + userId;
    std::filesystem::create_directories(basePath);

    std::filesystem::path filePath = std::filesystem::path(basePath) / (user_name + ".json");
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

// 타임스탬프 기준 항목 삭제 + 최신순 정렬된 JSON 저장
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
        if (entry.contains("timestamp")) {
            std::string ts = entry["timestamp"];
            if (ts == targetTimestamp) {
                std::wcout << L"[삭제됨] timestamp: " << Utf8ToUtf16(ts) << std::endl;
                deleted = true;
                continue;
            }
        }
        updatedContent.push_back(entry);
    }

    if (!deleted) {
        std::wcout << L"[알림] '" << Utf8ToUtf16(targetTimestamp) << L"' 타임스탬프 항목을 찾을 수 없습니다.\n";
        return;
    }

    // 🔽 최신순 정렬 (timestamp 기준 내림차순)
    std::sort(updatedContent.begin(), updatedContent.end(), [](const json& a, const json& b) {
        return a["timestamp"].get<std::string>() > b["timestamp"].get<std::string>();
        });

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        std::wcerr << L"[오류] 파일 저장 실패: " << Utf8ToUtf16(filePath.string()) << std::endl;
        return;
    }

    outFile << updatedContent.dump(4);  // 보기 좋게 저장
    outFile.close();

    std::wcout << L"✅ 타임스탬프 항목 삭제 및 정렬 완료: " << Utf8ToUtf16(filePath.string()) << std::endl;
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
        std::wcerr << L"CURL 초기화 실패\n";
        return { -1.0, -1.0 };
    }

    // 주소 인코딩
    char* encoded = curl_easy_escape(curl, address.c_str(), static_cast<int>(address.length()));
    if (!encoded) {
        std::wcerr << L"주소 인코딩 실패\n";
        curl_easy_cleanup(curl);
        return { -1.0, -1.0 };
    }

    std::string url = "https://dapi.kakao.com/v2/local/search/address.json?query=" + std::string(encoded);
    struct curl_slist* headers = nullptr;
    std::string api_key = "3e79cee032866364a65db09ff4d13f10";
    headers = curl_slist_append(headers, ("Authorization: KakaoAK " + api_key).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\Users\\lbh\\Desktop\\5team_project\\5team_server_0717\\cacert.pem");


    res = curl_easy_perform(curl);

    // 해제
    curl_free(encoded);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::wcerr << L"요청 실패: " << Utf8ToUtf16(curl_easy_strerror(res)) << std::endl;
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
        std::wcerr << L"JSON 파싱 오류: " << Utf8ToUtf16(e.what()) << std::endl;
    }

    return { -1.0, -1.0 }; // 실패 시
}

std::wstring Trim(const std::wstring& str) {
    const wchar_t* whitespace = L" \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::wstring::npos) ? L"" : str.substr(start, end - start + 1);
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
    if (clientIP != "127.0.0.1")
    {
        users_ip.push_back(clientIP);
    }
    // 2) 맵에 저장
    /*clients[clientIP] = clientSocket;*/
    
    std::wstring wClientIP = Utf8ToUtf16(clientIP);

    int clientPort = ntohs(addr.sin_port);

    // 2. 키 생성
    std::string key = clientIP + ":" + std::to_string(clientPort);

    // 3. 맵에 저장
    clientSockets[clientIP] = clientSocket;
    clientsList[clientIP].push_back(clientSocket);
    //clientsList[clientIP].push_back(clientSocket);
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
                        response["addr"] = userinfo.user_addr;
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
            else if (signal == "chat_reset")
            {
                std::unordered_map<std::string, bool> restartSentMap;
                //auto it = clientsList.find(targetIP);
                //if (it != clientsList.end() && !it->second.empty()) {
                //    // 예: 가장 먼저 connect 된 소켓에만 한 번 전송
                //    SOCKET sock = it->second.front();
                //    // 처음 접속 시에는 "상담 재시작" 메시지 먼저 보냄
                //    if (!restartSentMap[targetIP]) {
                //        response["prompt"] = "상담 재시작";
                //        sendJsonResponse(sock, response);
                //        restartSentMap[targetIP] = true;  // 전송 여부 기록

                //        std::wcout << L"[정보] 상담 재시작 메시지 전송됨" << "\n";
                //    }
                //}
                auto it = clientSockets.find(targetIP);
                if (it != clientSockets.end()) {
                    SOCKET sock = it->second;

                    if (!restartSentMap[targetIP]) {
                        response["prompt"] = "상담 재시작";
                        sendJsonResponse(sock, response);
                        restartSentMap[targetIP] = true;

                        std::wcout << L"[정보] 상담 재시작 메시지 전송됨" << "\n";
                    }
                }
                response["signal"] = "reset_ok";
                response["result"] = "0";
                sendJsonResponse(clientSocket, response);
            }
            else if (signal == "record")
            {
                //std::unordered_map<std::string, bool> restartSentMap;
                string question = request.value("say_text", "");
                
                std::wstring ques = Utf8ToUtf16(question);
                
                wprintf(L"질문: %ls\n", ques.c_str());
                
                //auto it = clientsList.find(targetIP);
                //if (it != clientsList.end() && !it->second.empty()) {
                //    // 예: 가장 먼저 connect 된 소켓에만 한 번 전송
                //    SOCKET sock = it->second.front();
                //    response["prompt"] = question;
                //    sendJsonResponse(sock, response);
                //}
                //else {
                //    std::cout << "[알림] 해당 IP의 소켓 리스트가 없습니다: "
                //        << targetIP << "\n";
                //}
                auto it = clientSockets.find(targetIP);
                if (it != clientSockets.end()) {
                    SOCKET sock = it->second;
                    response["prompt"] = question;
                    sendJsonResponse(sock, response);
                }
                else {
                    std::cout << "[알림] 해당 IP의 소켓이 등록되지 않았습니다: "
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

                std::string answer = request.value("result", "");
                std::wstring ai_answer = Utf8ToUtf16(answer);
                std::wstring change_answer = ai_answer;

                if (ai_answer.find(L"완화 방안:") != std::wstring::npos &&
                    ai_answer.find(L"추천 진료과:") != std::wstring::npos) {

                    std::wcout << L"[정보] 완화 방안 및 진료과 정보가 포함됨, 파싱 시작\n";

                    size_t advice_pos = ai_answer.find(L"완화 방안:");
                    size_t dept_pos = ai_answer.find(L"추천 진료과:");

                    std::wstring advice_text, department_text;

                    // 완화 방안 추출
                    if (advice_pos != std::wstring::npos) {
                        size_t end_pos = (dept_pos != std::wstring::npos) ? dept_pos : ai_answer.length();
                        advice_text = ai_answer.substr(advice_pos + 6, end_pos - (advice_pos + 6));
                        advice_text = Trim(advice_text);
                        std::wcout << L"완화 방안 원문: " << advice_text << std::endl;
                    }

                    // 추천 진료과 추출 (※ department_text 값을 반드시 설정)
                    if (dept_pos != std::wstring::npos) {
                        department_text = ai_answer.substr(dept_pos + 8);  // "추천 진료과:" 길이 8
                        department_text = Trim(department_text);
                        std::wcout << L"추천 진료과 원문: " << department_text << std::endl;
                    }

                    // 진료과 파싱
                    if (!department_text.empty()) {
                        std::wstringstream ss(department_text);
                        std::wstring token;

                        do {
                            std::getline(ss, token, L',');  // 쉼표 기준 (없으면 전체가 token)
                            token = Trim(token);

                            std::wcout << L"[디버그] 파싱된 진료과 토큰: [" << token << L"]" << std::endl;

                            if (!token.empty()) {
                                if (department_code_map.count(token)) {
                                    matched_departments.insert(token);
                                    matched_codes.push_back(department_code_map.at(token));
                                    std::wcout << L"→ 매칭된 진료과: " << token
                                        << L" / 코드: " << Utf8ToUtf16(department_code_map.at(token)) << std::endl;
                                }
                                else {
                                    std::wcout << L"⚠️ 미등록 진료과 토큰: [" << token << L"]" << std::endl;
                                }
                            }
                        } while (ss.good() && !ss.eof());
                    }

                }
                else {
                    std::wcout << L"[정보] 파싱 조건 미충족: 필요한 키워드가 없음\n";
                }

                // 사용자 세션 저장 (role: assistant)
                user_sessions[userIP].push_back({
                    "assistant",
                    answer
                    });

                // 클라이언트에 전송
                response["signal"] = "request";
                response["result"] = answer;

                for (const auto& ip : users_ip) {
                    auto it = clientsList.find(ip);
                    if (it != clientsList.end() && !it->second.empty()) {
                        // 첫 번째 소켓 사용 (필요 시 전체 소켓 순회 가능)
                        SOCKET sock = it->second.front();

                        std::wcout << L"소켓 찾음: IP = " << Utf8ToUtf16(ip)
                            << L", SOCKET = " << sock << std::endl;

                        // 예: JSON 전송 등 원하는 작업 수행
                        sendJsonResponse(sock, response);
                    }
                    else {
                        std::wcout << L"연결된 소켓이 없음: IP = " << Utf8ToUtf16(ip) << std::endl;
                    }
                }
            }   
            else if (signal == "save_chat")
            {
                string user_id = request.contains("id") ? request["id"].get<std::string>() : "";
                string user_name = request.contains("name") ? request["name"].get<std::string>() : "";
                json messagesJson = json::array(); // 기본값

                if (request.contains("messages") && request["messages"].is_array()) {
                    try {
                        for (const auto& item : request["messages"]) {
                            if (item.is_string()) {
                                messagesJson.push_back(item);
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        std::wcerr << L"❌ messages 파싱 중 예외: " << Utf8ToUtf16(e.what()) << std::endl;
                    }
                }
                else {
                    std::wcerr << L"⚠️ messages 키가 없거나 배열이 아님" << std::endl;
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
                string timestamp = request.value("timestamp", "");

                string userName = GetSafeFileName(user_name);

                deleteEntryByTimestamp(user_id, userName, timestamp);

                response["signal"] = "delete_result";
                response["result"] = "0";

                sendJsonResponse(clientSocket, response);

            }
            else if (signal == "reload_chat")
            {
                string user_id = request.value("id", "");
                string user_name = request.value("name", "");

                string userName = GetSafeFileName(user_name);

                // 기본 응답 구조
                response["signal"] = "reload_chat_result";
                response["data"] = json::array();  // 채팅 목록 배열로 초기화

                try {
                    // 경로 설정
                    std::string filePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + user_id + "\\" + userName + ".json";

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

                string userName = GetSafeFileName(user_name);

                json response;
                response["signal"] = "chat_detail_response";
                response["messages"] = json::array();  // 항상 배열로 초기화

                try {
                    std::string filePath = "C:\\Users\\lbh\\Desktop\\5team_project\\User_save_file\\" + user_id + "\\" + userName + ".json";

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
                //std::string found_addr = "";
                std::string found_addr = request.value("addr", "");

                std::wstring w_found_addr = Utf8ToUtf16(found_addr);
                std::wcout << L"[주소 (UTF-16)] " << w_found_addr << std::endl;

                // 좌표 추출 (string → double), 실패하면 -1.0 처리
                double inputLng = -1.0, inputLat = -1.0;
                try {
                    inputLng = std::stod(request.value("x", "0"));
                    inputLat = std::stod(request.value("y", "0"));
                }
                catch (...) {
                    inputLng = inputLat = -1.0;
                }

                // 좌표 유효하지 않으면 주소로부터 보완
                if ((inputLat <= 0.0 || inputLng <= 0.0) && !found_addr.empty()) {
                    std::tie(inputLng, inputLat) = getCoordinatesFromAddress(found_addr);  // X: 경도, Y: 위도
                }

                // 진료과 필터 생성
                std::string treat_filter_sql;
                if (!matched_departments.empty()) {
                    treat_filter_sql = " AND d.TREAT_CODE IN (";
                    size_t idx = 0;
                    for (const auto& dept : matched_departments) {
                        std::string utf8_dept = Utf16ToUtf8(dept);
                        treat_filter_sql += "'" + utf8_dept + "'";
                        if (++idx != matched_departments.size())
                            treat_filter_sql += ", ";
                    }
                    treat_filter_sql += ")";
                }

                if (inputLat > 0.0 && inputLng > 0.0) {
                    std::string query =
                        "SELECT "
                        "h.HOP_NAME, h.HOP_ADDR, h.X_CDN, h.Y_CDN, h.TYPE_CODE, "
                        "d.TREAT_CODE, d.NUM_SPC, h.HOP_PNUM, "
                        "("
                        "    6371 * acos("
                        "        cos(radians(" + std::to_string(inputLat) + ")) * cos(radians(h.Y_CDN)) * "
                        "        cos(radians(h.X_CDN) - radians(" + std::to_string(inputLng) + ")) + "
                        "        sin(radians(" + std::to_string(inputLat) + ")) * sin(radians(h.Y_CDN))"
                        "    )"
                        ") AS distance_km "
                        "FROM hospital_info h "
                        "JOIN hospital_detail d ON h.HOP_ID = d.HOP_ID "
                        + treat_filter_sql +
                        " GROUP BY h.HOP_ID "
                        " HAVING distance_km <= 3 "
                        "ORDER BY distance_km ASC;";

                    // MySQL 쿼리 실행
                    json hospital_info = json::array();
                    if (mysql_query(conn, query.c_str()) == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        MYSQL_ROW row;

                        if (res) {
                            while ((row = mysql_fetch_row(res))) {
                                json hospital = {
                                    {"name", row[0] ? row[0] : ""},
                                    {"address", row[1] ? row[1] : ""},
                                    {"x", row[2] ? row[2] : ""},
                                    {"y", row[3] ? row[3] : ""},
                                    {"type_code", row[4] ? row[4] : ""},
                                    {"treat_code", row[5] ? row[5] : ""},
                                    {"num_spc", row[6] ? row[6] : ""},
                                    {"phone", row[7] ? row[7] : ""},
                                    {"distance_km", row[8] ? row[8] : ""}
                                };
                                hospital_info.push_back(hospital);
                            }
                            mysql_free_result(res);

                            response["signal"] = "req_hop_result";
                            response["hospital"] = hospital_info;

                            std::string json_str = response.dump();  // JSON 객체를 문자열로 변환
                            std::wstring responses = Utf8ToUtf16(json_str);  // UTF-16으로 변환
                            std::wcout << L"[JSON 응답 (UTF-16)] " << responses << std::endl;
                            sendJsonResponse(clientSocket, response);
                        }
                        else {
                            std::wcerr << L"MySQL 결과 처리 오류: " << Utf8ToUtf16(mysql_error(conn)) << std::endl;
                        }
                    }
                    else {
                        std::wcerr << L"MySQL 쿼리 오류: " << Utf8ToUtf16(mysql_error(conn)) << std::endl;
                    }
                }
                else {
                    std::wcerr << L"좌표 유효성 실패: 좌표값 없음 또는 주소 변환 실패" << std::endl;
                    //response["signal"] = "req_hop_result";
                    //response["hospital"] = json::array();  // 빈 리스트 반환
                    //sendJsonResponse(clientSocket, response);
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

    if (recvBytes == 0) {
        std::wcout << L"클라이언트가 정상적으로 연결을 종료했습니다.\n";
    }
    else if (recvBytes == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        std::wcout << L"클라이언트 비정상 종료 또는 오류 (에러 코드: " << errorCode << L")\n";
    }

    // 소켓 종료
    closesocket(clientSocket);
    std::wcout << L"클라이언트 연결 종료\n";

    // 클라이언트 목록에서 제거
    {
        std::lock_guard<std::mutex> lock(clientsListMutex);

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


