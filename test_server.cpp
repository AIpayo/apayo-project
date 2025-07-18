//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <mysql.h>      // MariaDB/MySQL C API ���
//#include <iostream>
//#include <fcntl.h>
//#include <locale>
//#include <codecvt>
//#include <io.h>
//
//#pragma comment(lib, "ws2_32.lib")
////#pragma comment(lib, "libmysql.lib")  // libmariadb.lib �� ���� �ֽ��ϴ�
//#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")
//
//int PORT = 10000;
//
//// UTF-8 ���ڿ��� UTF-16���� ��ȯ�ؼ� ����ϴ� �Լ�
//void PrintUtf8AsUnicode(const char* utf8Str) {
//    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
//    if (len > 0) {
//        wchar_t* wstr = new wchar_t[len];
//        MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wstr, len);
//        std::wcout << L"Ŭ���̾�Ʈ �޽���: " << wstr << L"\n";
//        delete[] wstr;
//    }
//}
//
//int main() {
//    // --- 1) �ܼ� ����� UTF-16 ���� ����
//    _setmode(_fileno(stdout), _O_U16TEXT);
//    _setmode(_fileno(stderr), _O_U16TEXT);  // stderr �� ����
//    std::wcout << L"=== ���α׷� ���� ===" << std::endl;
//    std::wcerr << L"=== ���� ��Ʈ�� ���� ===" << std::endl;
//
//    std::wcout << L"���� �����... ��Ʈ1 " << PORT << L"\n"
//        << L"Ŭ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//    // --- 2) MariaDB ���� ����
//    MYSQL* conn = mysql_init(nullptr);
//    if (!conn) {
//        std::wcerr << L"mysql_init ����\n";
//        return 1;
//    }
//    std::wcout << L"���� �����... ��Ʈ2 " << PORT << L"\n"
//        << L"Ŭ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//    // DB ���� ����
//    const char* db_host = "10.10.20.109";
//    const char* db_user = "LBH";
//    const char* db_pass = "1234";
//    const char* db_name = "5TEAM_DB";
//    unsigned int db_port = 3306;
//
//    std::wcout << L"���� �����... ��Ʈ3 " << PORT << L"\n"
//        << L"Ŭ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//
//    if (!mysql_real_connect(conn,
//        db_host, db_user, db_pass,
//        db_name, db_port, nullptr, 0)) {
//        std::wstring err = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(mysql_error(conn));
//        std::wcerr << L"MariaDB ���� ����: " << err << L"\n";
//        mysql_close(conn);
//        return 1;
//    }
//    std::wcout << L"���� �����... ��Ʈ4 " << PORT << L"\n"
//        << L"Ŭ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//    std::wcout << L"MariaDB ���� ����\n";
//
//    // --- 3) WinSock �ʱ�ȭ
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::wcerr << L"WSAStartup ����\n";
//        mysql_close(conn);
//        return 1;
//    }
//
//    // --- 4) ���� ���� ���� & ���ε� & ����
//    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//    if (serverSocket == INVALID_SOCKET) {
//        std::wcerr << L"���� ���� ����\n";
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//
//    sockaddr_in serverAddr{};
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = INADDR_ANY;
//    serverAddr.sin_port = htons(PORT);
//
//    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::wcerr << L"���ε� ����\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//
//    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
//        std::wcerr << L"������ ����\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//
//    // ���
//    wchar_t bufMsg[100];
//    swprintf_s(bufMsg, L"���� �����... ��Ʈ %d\n", PORT);
//    std::wcout << bufMsg;
//
//    std::wcout << L"���� �����... ��Ʈ " << PORT << L"\n"
//        << L"Ŭ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//    // --- 5) Ŭ���̾�Ʈ ó�� ����
//    while (true) {
//        sockaddr_in clientAddr;
//        int clientAddrSize = sizeof(clientAddr);
//        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
//        if (clientSocket == INVALID_SOCKET) {
//            std::wcerr << L"Ŭ���̾�Ʈ ���� ����\n";
//            continue;
//        }
//        std::wcout << L"Ŭ���̾�Ʈ ���ӵ�!\n";
//
//        char buffer[1024];
//        int recvBytes = 0;
//        while ((recvBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
//            buffer[recvBytes] = '\0';
//            PrintUtf8AsUnicode(buffer);
//
//            // --- 6) ���� �޽����� DB�� INSERT (����)
//            {
//                std::string msg(buffer);
//                // ������ escape ó�� ���� ������ ����ó��, ������ SQL Injection ���� �ʿ�
//                std::string query = "INSERT INTO messages (content) VALUES ('" + msg + "')";
//                if (mysql_query(conn, query.c_str()) != 0) {
//                    std::wcerr << L"INSERT ����: "
//                        << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(mysql_error(conn))
//                        << L"\n";
//                }
//                else {
//                    std::wcout << L"DB�� �޽��� ���� �Ϸ�\n";
//                }
//            }
//        }
//
//        if (recvBytes == 0) {
//            std::wcout << L"Ŭ���̾�Ʈ ���� ����\n";
//        }
//        else {
//            std::wcerr << L"recv ����\n";
//        }
//
//        closesocket(clientSocket);
//        std::wcout << L"Ŭ���̾�Ʈ ���� ����\n";
//    }
//
//    // --- 7) ����
//    closesocket(serverSocket);
//    WSACleanup();
//    mysql_close(conn);
//    return 0;
//}

//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <mysql.h>      // MariaDB/MySQL C API ���
//#include <iostream>
//#include <fcntl.h>
//#include <locale>
//#include <codecvt>
//#include <io.h>
//#include <vector>
//#include <sstream>
//#include <nlohmann/json.hpp>  // https://github.com/nlohmann/json
//
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")
//
//using namespace std;
//using json = nlohmann::json;
//int PORT = 10000;
//static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
//
//// ���α׷� ���� �� �� ���� ȣ��
//void init_console_utf8() {
//    // �ܼ� ���, �Է� ��� UTF-8�� ����
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//}
//
//// UTF-8 ���ڿ��� UTF-16���� ��ȯ�ؼ� ����ϴ� �Լ�
//void PrintUtf8AsUnicode(const char* utf8Str) {
//    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
//    if (len > 0) {
//        wchar_t* wstr = new wchar_t[len];
//        MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wstr, len);
//        std::wcout << L"Ŭ���̾�Ʈ �޽���: " << wstr << L"\n";
//        delete[] wstr;
//    }
//}
//
//// UTF-8(std::string) -> UTF-16(std::wstring) ��ȯ helper
//std::wstring Utf8ToUtf16(const std::string& utf8) {
//    if (utf8.empty()) return {};
//    // ��ȯ�� UTF-16 ����(���� ���� + �� ����)�� ����
//    int size_needed = MultiByteToWideChar(
//        CP_UTF8, 0,
//        utf8.c_str(), -1,
//        nullptr, 0
//    );
//    std::wstring wstr(size_needed, L'\0');
//    // ���� ��ȯ
//    MultiByteToWideChar(
//        CP_UTF8, 0,
//        utf8.c_str(), -1,
//        &wstr[0], size_needed
//    );
//    // ���� �� ���� ����
//    if (!wstr.empty() && wstr.back() == L'\0')
//        wstr.pop_back();
//    return wstr;
//}
//
//// ���ڿ� escape helper
//std::string escapeString(MYSQL* conn, const std::string& str) {
//    char* buf = new char[str.size() * 2 + 1];
//    unsigned long len = mysql_real_escape_string(conn, buf, str.c_str(), (unsigned long)str.size());
//    std::string ret(buf, len);
//    delete[] buf;
//    return ret;
//}
//
//// ���͸� �����ڿ� �Բ� �̾���̱�
//std::string join(const std::vector<std::string>& vec, const char* delim) {
//    std::ostringstream oss;
//    for (size_t i = 0; i < vec.size(); ++i) {
//        if (i) oss << delim;
//        oss << vec[i];
//    }
//    return oss.str();
//}
//
//// ���� JSON�� signal, message�� �Բ� Ŭ���̾�Ʈ�� �����ϴ� ���� �Լ�
//void sendJsonResponse(SOCKET client, json resp) {
//    std::string out = resp.dump() + "\n";
//        const char* p = out.c_str();
//    int total = (int)out.size();
//    int sent = 0;
//    while (sent < total) {
//        int r = send(client, p + sent, total - sent, 0);
//        if (r == SOCKET_ERROR) {
//            std::wcerr << L"send ����";
//            break;
//        }
//        sent += r;
//    }
//}
//
//int main() {
//    // --- �ܼ� ����� UTF-16 ���� ����
//    _setmode(_fileno(stdout), _O_U16TEXT);
//    _setmode(_fileno(stderr), _O_U16TEXT);
//    std::wcout << L"=== ���α׷� ���� ===" << std::endl;
//
//    // MariaDB ���� ����
//    MYSQL* conn = mysql_init(nullptr);
//    if (!conn) {
//        std::wcerr << L"mysql_init ����\n";
//        return 1;
//    }
//    const char* db_host = "10.10.20.105";
//    const char* db_user = "USER";
//    const char* db_pass = "1234";
//    const char* db_name = "apayo";
//    unsigned int db_port = 3306;
//
//    if (!mysql_real_connect(conn, db_host, db_user, db_pass, db_name, db_port, nullptr, 0)) {
//        std::wstring err = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(mysql_error(conn));
//        std::wcerr << L"MariaDB ���� ����: " << err << L"\n";
//        mysql_close(conn);
//        return 1;
//    }
//    std::wcout << L"MariaDB ���� ����\n";
//
//    // WinSock �ʱ�ȭ
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::wcerr << L"WSAStartup ����\n";
//        mysql_close(conn);
//        return 1;
//    }
//
//    // ���� ���� ���� & ���ε� & ����
//    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//    if (serverSocket == INVALID_SOCKET) {
//        std::wcerr << L"���� ���� ����\n";
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//    sockaddr_in serverAddr{};
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = INADDR_ANY;
//    serverAddr.sin_port = htons(PORT);
//    if (::bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::wcerr << L"���ε� ����\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
//        std::wcerr << L"������ ����\n";
//        closesocket(serverSocket);
//        WSACleanup();
//        mysql_close(conn);
//        return 1;
//    }
//    std::wcout << L"���� �����... ��Ʈ " << PORT << L"\nŬ���̾�Ʈ ������ ��ٸ��ϴ١�" << std::endl;
//
//    // Ŭ���̾�Ʈ ó�� ����
//    while (true) {
//        sockaddr_in clientAddr;
//        int clientAddrSize = sizeof(clientAddr);
//        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
//        if (clientSocket == INVALID_SOCKET) {
//            std::wcerr << L"Ŭ���̾�Ʈ ���� ����\n";
//            continue;
//        }
//        std::wcout << L"Ŭ���̾�Ʈ ���ӵ�!\n";
//
//        char buffer[4096];
//        int recvBytes = 0;
//        while ((recvBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
//            buffer[recvBytes] = '\0';
//            PrintUtf8AsUnicode(buffer);
//
//            // JSON �Ľ� �� CRUD
//            json request;
//            json response;
//            try {
//                request = json::parse(buffer);
//                std::string signal = request.value("signal", "");
//                std::string table = request.value("table", "messages");
//
//                if (signal == "create") {
//                    auto data = request.at("data");
//                    std::vector<std::string> fields, values;
//                    for (auto& kv : data.items()) {
//                        fields.push_back(kv.key());
//                        values.push_back("'" + escapeString(conn, kv.value().get<std::string>()) + "'");
//                    }
//                    std::string query = "INSERT INTO " + table + " (" + join(fields, ",") + ") VALUES (" + join(values, ",") + ")";
//                    if (mysql_query(conn, query.c_str()) == 0) {
//                        response["status"] = "success";
//                        response["insert_id"] = mysql_insert_id(conn);
//                    }
//                    else {
//                        response["status"] = "error";
//                        response["error"] = mysql_error(conn);
//                    }
//                }
//                else if (signal == "join")
//                {
//
//                    wprintf(L"here join\n");
//
//                    string id = request.value("id", "");
//                    string pw = request.value("pw", "");
//                    string name = request.value("name", std::string{});
//                    string addr = request.value("adress", std::string{});
//                    string phone = request.value("phone", "");
//                    string birth = request.value("year", "");
//                    string sex = request.value("gender", std::string{});
//                    wstring addrs = converter.from_bytes(addr);
//                    
//                    string gender;
//
//                    if (sex == "����")
//                    {
//                        gender = "F";
//                    }
//                    else
//                    {
//                        gender = "M";
//                    }
//
//                    wprintf(L"here join: %ls\n", addrs.c_str());
//                   
//                    string join_query = "INSERT INTO user_info(USER_ID, USER_PW, USER_NAME, USER_ADDR, USER_PNUM, USER_BIRTH, USER_SEX)"
//                        "VALUES('"+id+"','"+pw+"','"+name+"','" + addr + "','" + phone + "','" + birth + "','" + gender + "')";
//
//                    if (mysql_query(conn, join_query.c_str()) == 0) {
//                        response["signal"] = "join_success";
//                        response["result"] = "0";
//                        sendJsonResponse(clientSocket, response);
//                    }
//                    else
//                    {
//                        wprintf(L"Query Error");
//                    }
//
//
//                }
//                else if (signal == "login") {
//                    string id = request.value("id", "");
//                    string pw = request.value("pw", "");
//
//                    //std::wstring wid = converter.from_bytes(id);
//                    //std::wstring wpw = converter.from_bytes(pw);
//
//                    //wprintf(L"ID: %ls\n", wid.c_str());
//                    //wprintf(L"PW: %ls\n", wpw.c_str());
//
//                    string id_query = "SELECT * FROM user_info WHERE USER_ID = '"+id+"' AND USER_PW = '"+pw+"'";
//                    
//                    if (mysql_query(conn, id_query.c_str()) == 0) {
//                        MYSQL_RES* result = mysql_store_result(conn);
//
//                        // ��� ���� �Ǵ� ����ִ� ���
//                        if (!result || mysql_num_rows(result) == 0) {
//                            response["signal"] = "login_result";
//                            response["result"] = "1";
//                            sendJsonResponse(clientSocket, response);
//                            if (result) mysql_free_result(result);
//                            continue;
//                        }
//
//                        response["signal"] = "login_result";
//                        response["result"] = "0";
//                        sendJsonResponse(clientSocket, response);
//                        
//                        mysql_free_result(result);
//
//                    }
//                    else
//                    {
//                        wprintf(L"Login Query Error\n");
//                    }
//                    
//                }
//                else if (signal == "update") {
//                    int id = request.value("id", 0);
//                    auto data = request.at("data");
//                    std::vector<std::string> sets;
//                    for (auto& kv : data.items()) {
//                        sets.push_back(kv.key() + "='" + escapeString(conn, kv.value().get<std::string>()) + "'");
//                    }
//                    std::string query = "UPDATE " + table + " SET " + join(sets, ",") + " WHERE id=" + std::to_string(id);
//                    if (mysql_query(conn, query.c_str()) == 0) {
//                        response["status"] = "success";
//                        response["affected_rows"] = mysql_affected_rows(conn);
//                    }
//                    else {
//                        response["status"] = "error";
//                        response["error"] = mysql_error(conn);
//                    }
//                }
//                else if (signal == "delete") {
//                    int id = request.value("id", 0);
//                    std::string query = "DELETE FROM " + table + " WHERE id=" + std::to_string(id);
//                    if (mysql_query(conn, query.c_str()) == 0) {
//                        response["status"] = "success";
//                        response["affected_rows"] = mysql_affected_rows(conn);
//                    }
//                    else {
//                        response["status"] = "error";
//                        response["error"] = mysql_error(conn);
//                    }
//                }
//                else if (signal == "record")
//                {
//                    string question = request.value("say_text", "");
//
//                    std::wstring ques = converter.from_bytes(question);
//
//                    wprintf(L"����: %ls\n", ques.c_str());
//
//                    response["signal"] = "request";
//                    response["result"] = "��";
//                    sendJsonResponse(clientSocket, response);
//                }
//                else {
//                    response["status"] = "error";
//                    response["error"] = "Invalid action";
//                }
//            }
//            catch (json::exception& e) {
//                response["status"] = "error";
//                response["error"] = std::string("JSON parse error: ") + e.what();
//            }
//
//            std::string out = response.dump();
//            send(clientSocket, out.c_str(), (int)out.size(), 0);
//        }
//
//        if (recvBytes == 0) {
//            std::wcout << L"Ŭ���̾�Ʈ ���� ����\n";
//        }
//        else {
//            std::wcerr << L"recv ����\n";
//        }
//
//        closesocket(clientSocket);
//        std::wcout << L"Ŭ���̾�Ʈ ���� ����\n";
//    }
//
//    // ����
//    closesocket(serverSocket);
//    WSACleanup();
//    mysql_close(conn);
//    return 0;
//}
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600  // Windows Vista �̻� Ÿ����

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mysql.h>      // MariaDB/MySQL C API ���
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <locale>
#include <codecvt>
#include <vector>
#include <sstream>
#include <thread>
#include <nlohmann/json.hpp>  // https://github.com/nlohmann/json

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")

using namespace std;
using json = nlohmann::json;

static const int PORT = 10000;
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

// �ܼ��� UTF-8 ���� ����
void init_console_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
}

// UTF-8 �� UTF-16 ��ȯ �� ���
void PrintUtf8AsUnicode(const char* utf8Str) {
    std::wstring wstr = converter.from_bytes(utf8Str);
    std::wcout << wstr << L"\n";
}

// JSON ���� ���� ����
void sendJsonResponse(SOCKET client, const json& resp) {
    string out = resp.dump() + "\n";
    const char* p = out.c_str();
    int total = (int)out.size(), sent = 0;
    while (sent < total) {
        int r = send(client, p + sent, total - sent, 0);
        if (r == SOCKET_ERROR) {
            std::wcerr << L"send ����\n";
            break;
        }
        sent += r;
    }
}

// ���ڿ� �̽�������
string escapeString(MYSQL* conn, const string& str) {
    char* buf = new char[str.size() * 2 + 1];
    unsigned long len = mysql_real_escape_string(conn, buf, str.c_str(), (unsigned long)str.size());
    string ret(buf, len);
    delete[] buf;
    return ret;
}

// ���� join
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

// Ŭ���̾�Ʈ ó�� �Լ�
void handleClient(SOCKET clientSocket, MYSQL* conn) {
    // Ŭ���̾�Ʈ IP ���
    sockaddr_in addr{};
    int addrlen = sizeof(addr);
    getpeername(clientSocket, reinterpret_cast<sockaddr*>(&addr), &addrlen);

    // InetNtopA�� IPv4 ���ڿ� ��ȯ
    char ipStr[INET_ADDRSTRLEN] = {};
    InetNtopA(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
    wprintf(L"Ŭ���̾�Ʈ ���ӵ�: %hs\n", ipStr);

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
                 
                if (sex == "����")
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
                        response["result"] = "1";  // �α��� ����
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
                
                wprintf(L"����: %ls\n", ques.c_str());
                
                response["signal"] = "request";
                response["result"] = converter.to_bytes(L"��");
                sendJsonResponse(clientSocket, response);
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
    std::wcout << L"Ŭ���̾�Ʈ ���� ����\n";
}

int main() {
    init_console_utf8();
    std::wcout << L"=== ���α׷� ���� ===\n";

    // MariaDB ����
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        std::wcerr << L"mysql_init ����\n";
        return 1;
    }
    if (!mysql_real_connect(conn,
        "10.10.20.105",
        "USER",
        "1234",
        "apayo",
        3306, nullptr, 0)) {
        std::wcerr << L"DB ���� ����: " << converter.from_bytes(mysql_error(conn)) << L"\n";
        mysql_close(conn);
        return 1;
    }
    std::wcout << L"MariaDB ���� ����\n";

    // WinSock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::wcerr << L"WSAStartup ����\n";
        mysql_close(conn);
        return 1;
    }

    // ���� ���� ����
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::wcerr << L"���� ���� ����\n";
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (::bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::wcerr << L"���ε� ����\n";
        closesocket(serverSocket);
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::wcerr << L"������ ����\n";
        closesocket(serverSocket);
        WSACleanup();
        mysql_close(conn);
        return 1;
    }
    std::wcout << L"���� �����... ��Ʈ " << PORT << L"\n";

    // ���� Ŭ���̾�Ʈ ó��
    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = ::accept(
            serverSocket,
            reinterpret_cast<SOCKADDR*>(&clientAddr),
            &clientAddrSize
        );
        if (clientSocket == INVALID_SOCKET) {
            std::wcerr << L"Ŭ���̾�Ʈ ���� ����\n";
            continue;
        }
        // ������� �б� ó��
        std::thread t(handleClient, clientSocket, conn);
        t.detach();
    }

    // �� �ڵ�� �������� ����
    closesocket(serverSocket);
    WSACleanup();
    mysql_close(conn);
    return 0;
}
