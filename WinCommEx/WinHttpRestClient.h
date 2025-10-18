#pragma once
#include "Export.h"

#include <Windows.h>
#include <winhttp.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#pragma region("Sample Code")
#if FALSE
#include "WinHttpRestClient.h"

void SampleRestCall()
{
    CWinHttpRestClient WinHttpClient;
    if (!WinHttpClient.Initialize(L"https://api.example.com", L"LBStudio/1.0"))
    {
        DWORD dwErr = WinHttpClient.GetLastErrorCode();
        // TODO: Handle logging
        UNREFERENCED_PARAMETER(dwErr);
        return;
    }

    WinHttpClient.SetTimeouts({ 3000, 3000, 5000, 5000 });
    WinHttpClient.AddDefaultHeader(L"Accept", L"application/json");

    CWinHttpRestResponse Response;
    if (WinHttpClient.Get(L"/v1/status", Response))
    {
        const DWORD dwStatus = Response.GetStatusCode();
        const std::string& strBody = Response.GetBody();
        UNREFERENCED_PARAMETER(dwStatus);
        UNREFERENCED_PARAMETER(strBody);

        std::wstring wstrServerDate;
        if (Response.TryGetHeader(L"Date", wstrServerDate))
        {
            std::wstring wstrMessage = L"Server Date: ";
            wstrMessage += wstrServerDate;
            wstrMessage += L"\r\n";
            OutputDebugStringW(wstrMessage.c_str());
        }
    }
    else
    {
        DWORD dwErr = WinHttpClient.GetLastErrorCode();
        // TODO: Handle failure
        UNREFERENCED_PARAMETER(dwErr);
    }

    // POST sample
    const std::string strPayload = R"({"message":"hello"})";
    Response.Reset();
    if (WinHttpClient.Post(L"/v1/messages", strPayload, Response))
    {
        // TODO: Handle success
    }
    else
    {
        DWORD dwErr = WinHttpClient.GetLastErrorCode();
        UNREFERENCED_PARAMETER(dwErr);
    }

    WinHttpClient.Cleanup();
}

#endif
#pragma endregion

struct CWinHttpTimeouts
{
    DWORD dwResolve = 5000;
    DWORD dwConnect = 5000;
    DWORD dwSend = 10000;
    DWORD dwReceive = 10000;
};

class WINCOMMEX_API CWinHttpRestResponse
{
public:
    void Reset();

    void SetStatusCode(DWORD dwCode);
    DWORD GetStatusCode() const;

    void SetBody(std::string strBody);
    const std::string& GetBody() const;
    void AppendBody(const std::string& strChunk);

    void SetRawHeaders(std::wstring wstrHeaders);
    const std::wstring& GetRawHeaders() const;

    const std::vector<std::pair<std::wstring, std::wstring>>& GetHeaders() const;
    bool TryGetHeader(const std::wstring& wstrName, std::wstring& wstrValue) const;
    void SetHeaders(std::vector<std::pair<std::wstring, std::wstring>> vHeaders);
    void AddHeader(std::wstring wstrName, std::wstring wstrValue);

private:
    void ParseRawHeaders();
    static std::wstring Trim(const std::wstring& wstrText);

    DWORD m_dwStatusCode = 0;
    std::wstring m_wstrRawHeaders;
    std::vector<std::pair<std::wstring, std::wstring>> m_vHeaders;
    std::string m_strBody;
};

class WINCOMMEX_API CWinHttpRestClient
{
public:
    CWinHttpRestClient();
    ~CWinHttpRestClient();

    bool Initialize(const std::wstring& wstrBaseUrl, const std::wstring& wstrUserAgent = L"WinHTTP/1.0");
    bool Get(const std::wstring& wstrPath, CWinHttpRestResponse& Response, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders = {});
    bool Post(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType = L"application/json", const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders = {});
    bool Put(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType = L"application/json", const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders = {});
    bool Patch(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType = L"application/json", const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders = {});
    bool Delete(const std::wstring& wstrPath, CWinHttpRestResponse& Response, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders = {});
    void Cleanup();

    void SetTimeouts(const CWinHttpTimeouts& Timeouts);
    CWinHttpTimeouts GetTimeouts() const;

    void SetDefaultHeaders(std::vector<std::pair<std::wstring, std::wstring>> vHeaders);
    void AddDefaultHeader(std::wstring wstrName, std::wstring wstrValue);
    void ClearDefaultHeaders();

    DWORD GetLastErrorCode() const;

private:
    bool EnsureConnection();
    bool Reconnect();
    bool SendRequest(const std::wstring& wstrMethod, const std::wstring& wstrPath, CWinHttpRestResponse& Response, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders, const std::string* pstrBody, const std::wstring& wstrContentType);
    std::wstring BuildPath(const std::wstring& wstrPath) const;
    void ApplyTimeouts();
    void CleanupUnlocked();

    std::wstring m_wstrHost;
    std::wstring m_wstrBasePath;
    INTERNET_PORT m_nPort;
    bool m_bUseTls;

    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    std::wstring m_wstrUserAgent;
    CWinHttpTimeouts m_Timeouts;
    std::vector<std::pair<std::wstring, std::wstring>> m_vDefaultHeaders;
    DWORD m_dwLastError;
    mutable CRITICAL_SECTION m_csLock;
};

