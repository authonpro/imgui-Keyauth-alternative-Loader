#pragma once
// ============================================================
//  AUTHON SDK - C++ Header-Only (WinHTTP)
//  https://authon.pro/docs/sdk-cpp
// ============================================================

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

namespace authon {

struct AppInfo {
    std::string name;
    std::string version;
    bool initialized = false;
};

struct UserInfo {
    std::string username;
    int level = 0;
    std::string expiresAt;
    std::string subscription;
};

class Authon {
private:
    std::string appId, apiKey, baseUrl, sessionToken;
    AppInfo app;
    UserInfo user;

    std::string httpPost(const std::string& path, const std::string& body) {
        URL_COMPONENTSW urlComp = {};
        wchar_t hostBuf[256] = {}, pathBuf[1024] = {};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostBuf;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = pathBuf;
        urlComp.dwUrlPathLength = 1024;

        std::wstring wUrl(baseUrl.begin(), baseUrl.end());
        std::wstring wPath(path.begin(), path.end());
        wUrl += wPath;

        WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp);

        HINTERNET hSession = WinHttpOpen(L"AuthonSDK/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) return "";

        HINTERNET hConnect = WinHttpConnect(hSession, hostBuf, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", pathBuf, NULL, NULL, NULL,
            urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

        std::wstring headers = L"Content-Type: application/json\r\n";
        WinHttpSendRequest(hRequest, headers.c_str(), -1, (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
        WinHttpReceiveResponse(hRequest, NULL);

        std::string response;
        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    std::string httpGet(const std::string& path) {
        URL_COMPONENTSW urlComp = {};
        wchar_t hostBuf[256] = {}, pathBuf[1024] = {};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostBuf;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = pathBuf;
        urlComp.dwUrlPathLength = 1024;

        std::wstring wUrl(baseUrl.begin(), baseUrl.end());
        std::wstring wPath(path.begin(), path.end());
        wUrl += wPath;

        WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp);

        HINTERNET hSession = WinHttpOpen(L"AuthonSDK/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) return "";

        HINTERNET hConnect = WinHttpConnect(hSession, hostBuf, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

        std::string tokenHeader = "X-Session-Token: " + sessionToken;
        std::wstring wHeaders(tokenHeader.begin(), tokenHeader.end());
        wHeaders += L"\r\n";

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pathBuf, NULL, NULL, NULL,
            urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

        WinHttpSendRequest(hRequest, wHeaders.c_str(), -1, NULL, 0, 0, 0);
        WinHttpReceiveResponse(hRequest, NULL);

        std::string response;
        DWORD bytesRead = 0;
        char buffer[8192];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    std::string jsonValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":";
        auto pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
        if (pos > 0 && json[pos - 1] == '"') {
            auto end = json.find('"', pos);
            return json.substr(pos, end - pos);
        }
        auto end = json.find_first_of(",}", pos);
        return json.substr(pos, end - pos);
    }

    bool jsonBool(const std::string& json, const std::string& key) {
        return jsonValue(json, key) == "true";
    }

public:
    Authon(const std::string& id, const std::string& key, const std::string& url)
        : appId(id), apiKey(key), baseUrl(url) {}

    bool init() {
        std::string body = "{\"type\":\"init\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey + "\"}";
        std::string res = httpPost("/v1", body);
        if (res.empty()) return false;
        if (!jsonBool(res, "success")) return false;
        app.name = jsonValue(res, "name");
        app.version = jsonValue(res, "version");
        app.initialized = true;
        return true;
    }

    bool login(const std::string& username, const std::string& password, const std::string& hwid = "") {
        std::string body = "{\"type\":\"login\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey +
            "\",\"username\":\"" + username + "\",\"password\":\"" + password + "\"";
        if (!hwid.empty()) body += ",\"hwid\":\"" + hwid + "\"";
        body += "}";
        std::string res = httpPost("/v1", body);
        if (!jsonBool(res, "success")) return false;
        sessionToken = jsonValue(res, "sessionToken");
        user.username = jsonValue(res, "username");
        user.level = std::stoi(jsonValue(res, "level").empty() ? "1" : jsonValue(res, "level"));
        user.expiresAt = jsonValue(res, "expiresAt");
        user.subscription = jsonValue(res, "subscription");
        return true;
    }

    bool license(const std::string& key, const std::string& hwid = "") {
        std::string body = "{\"type\":\"license\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey +
            "\",\"licenseKey\":\"" + key + "\"";
        if (!hwid.empty()) body += ",\"hwid\":\"" + hwid + "\"";
        body += "}";
        std::string res = httpPost("/v1", body);
        if (!jsonBool(res, "success")) return false;
        sessionToken = jsonValue(res, "sessionToken");
        user.username = jsonValue(res, "username");
        user.level = std::stoi(jsonValue(res, "level").empty() ? "1" : jsonValue(res, "level"));
        return true;
    }

    bool registerUser(const std::string& username, const std::string& password, const std::string& key, const std::string& hwid = "") {
        std::string body = "{\"type\":\"register\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey +
            "\",\"username\":\"" + username + "\",\"password\":\"" + password +
            "\",\"licenseKey\":\"" + key + "\"";
        if (!hwid.empty()) body += ",\"hwid\":\"" + hwid + "\"";
        body += "}";
        std::string res = httpPost("/v1", body);
        if (!jsonBool(res, "success")) return false;
        sessionToken = jsonValue(res, "sessionToken");
        user.username = username;
        return true;
    }

    std::vector<unsigned char> downloadFile(const std::string& fileId) {
        // KeyAuth style: POST /v1 with type=file, returns raw bytes directly
        std::string body = "{\"type\":\"file\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey +
            "\",\"sessionToken\":\"" + sessionToken + "\",\"fileId\":\"" + fileId + "\"}";

        URL_COMPONENTSW urlComp = {};
        wchar_t hostBuf[256] = {}, pathBuf[1024] = {};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostBuf; urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = pathBuf; urlComp.dwUrlPathLength = 1024;

        std::wstring wUrl(baseUrl.begin(), baseUrl.end());
        wUrl += L"/v1";
        WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp);

        HINTERNET hSession = WinHttpOpen(L"AuthonSDK/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) return {};
        HINTERNET hConnect = WinHttpConnect(hSession, hostBuf, urlComp.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", pathBuf, NULL, NULL, NULL,
            urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

        std::wstring headers = L"Content-Type: application/json\r\n";
        WinHttpSendRequest(hRequest, headers.c_str(), -1, (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
        WinHttpReceiveResponse(hRequest, NULL);

        // Check status
        DWORD statusCode = 0, statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusSize, NULL);

        std::vector<unsigned char> data;
        if (statusCode == 200) {
            DWORD bytesRead = 0;
            unsigned char buffer[8192];
            while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                data.insert(data.end(), buffer, buffer + bytesRead);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // If small response with JSON error, discard
        if (data.size() < 200 && data.size() > 0) {
            std::string check(data.begin(), data.end());
            if (check.find("\"success\"") != std::string::npos && check.find("false") != std::string::npos) {
                return {};
            }
        }

        return data;
    }

    void log(const std::string& message) {
        std::string body = "{\"type\":\"log\",\"appId\":\"" + appId + "\",\"apiKey\":\"" + apiKey +
            "\",\"logType\":\"ACTIVITY\",\"message\":\"" + message + "\"}";
        httpPost("/v1", body);
    }

    AppInfo getApp() const { return app; }
    UserInfo getUser() const { return user; }
    std::string getSessionToken() const { return sessionToken; }
};

} // namespace authon
