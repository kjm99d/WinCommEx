#include "WinHttpRestClient.h"

#include <cwctype>
#include <cwchar>

#pragma comment(lib, "winhttp.lib")

namespace
{
class CCriticalSectionGuard
{
public:
    explicit CCriticalSectionGuard(CRITICAL_SECTION& CriticalSection) : m_CriticalSection(CriticalSection) { EnterCriticalSection(&m_CriticalSection); }
    ~CCriticalSectionGuard() { LeaveCriticalSection(&m_CriticalSection); }

private:
    CRITICAL_SECTION& m_CriticalSection;
};
}

void CWinHttpRestResponse::Reset()
{
    m_dwStatusCode = 0;
    m_wstrRawHeaders.clear();
    m_vHeaders.clear();
    m_strBody.clear();
}

void CWinHttpRestResponse::SetStatusCode(DWORD dwCode)
{
    m_dwStatusCode = dwCode;
}

DWORD CWinHttpRestResponse::GetStatusCode() const
{
    return m_dwStatusCode;
}

void CWinHttpRestResponse::SetBody(std::string strBody)
{
    m_strBody = std::move(strBody);
}

const std::string& CWinHttpRestResponse::GetBody() const
{
    return m_strBody;
}

void CWinHttpRestResponse::AppendBody(const std::string& strChunk)
{
    m_strBody.append(strChunk);
}

void CWinHttpRestResponse::SetRawHeaders(std::wstring wstrHeaders)
{
    m_wstrRawHeaders = std::move(wstrHeaders);
    ParseRawHeaders();
}

const std::wstring& CWinHttpRestResponse::GetRawHeaders() const
{
    return m_wstrRawHeaders;
}

const std::vector<std::pair<std::wstring, std::wstring>>& CWinHttpRestResponse::GetHeaders() const
{
    return m_vHeaders;
}

void CWinHttpRestResponse::SetHeaders(std::vector<std::pair<std::wstring, std::wstring>> vHeaders)
{
    m_vHeaders = std::move(vHeaders);
}

void CWinHttpRestResponse::AddHeader(std::wstring wstrName, std::wstring wstrValue)
{
    m_vHeaders.emplace_back(std::move(wstrName), std::move(wstrValue));
}

bool CWinHttpRestResponse::TryGetHeader(const std::wstring& wstrName, std::wstring& wstrValue) const
{
    for (const auto& Header : m_vHeaders)
    {
        if (_wcsicmp(Header.first.c_str(), wstrName.c_str()) == 0)
        {
            wstrValue = Header.second;
            return true;
        }
    }
    return false;
}

void CWinHttpRestResponse::ParseRawHeaders()
{
    m_vHeaders.clear();

    if (m_wstrRawHeaders.empty())
        return;

    size_t nStart = 0;
    size_t nEnd = 0;
    bool bFirstLine = true;
    while (nStart < m_wstrRawHeaders.length())
    {
        nEnd = m_wstrRawHeaders.find(L"\r\n", nStart);
        if (nEnd == std::wstring::npos)
            nEnd = m_wstrRawHeaders.length();

        std::wstring wstrLine = m_wstrRawHeaders.substr(nStart, nEnd - nStart);
        if (!wstrLine.empty())
        {
            if (bFirstLine)
            {
                // Skip the HTTP status line.
                bFirstLine = false;
            }
            else
            {
                size_t nColon = wstrLine.find(L':');
                if (nColon != std::wstring::npos)
                {
                    std::wstring wstrName = Trim(wstrLine.substr(0, nColon));
                    std::wstring wstrValue = Trim(wstrLine.substr(nColon + 1));
                    if (!wstrName.empty())
                        AddHeader(std::move(wstrName), std::move(wstrValue));
                }
            }
        }

        if (nEnd == m_wstrRawHeaders.length())
            break;
        nStart = nEnd + 2; // "\r\n"
    }
}

std::wstring CWinHttpRestResponse::Trim(const std::wstring& wstrText)
{
    size_t nBegin = 0;
    size_t nEnd = wstrText.length();
    while (nBegin < nEnd && iswspace(wstrText[nBegin]))
        ++nBegin;

    while (nEnd > nBegin && iswspace(wstrText[nEnd - 1]))
        --nEnd;

    return wstrText.substr(nBegin, nEnd - nBegin);
}


CWinHttpRestClient::CWinHttpRestClient()
    : m_nPort(INTERNET_DEFAULT_HTTPS_PORT)
    , m_bUseTls(true)
    , m_hSession(nullptr)
    , m_hConnect(nullptr)
    , m_Timeouts{}
    , m_dwLastError(ERROR_SUCCESS)
{
    InitializeCriticalSection(&m_csLock);
}

CWinHttpRestClient::~CWinHttpRestClient()
{
    Cleanup();
    DeleteCriticalSection(&m_csLock);
}

bool CWinHttpRestClient::Initialize(const std::wstring& wstrBaseUrl, const std::wstring& wstrUserAgent)
{
    CCriticalSectionGuard Guard(m_csLock);
    CleanupUnlocked();
    m_dwLastError = ERROR_SUCCESS;

    std::wstring wstrNormalized = wstrBaseUrl;
    if (wstrNormalized.find(L"://") == std::wstring::npos)
        wstrNormalized = L"https://" + wstrNormalized;

    URL_COMPONENTS Components = {};
    Components.dwStructSize = sizeof(Components);
    Components.dwSchemeLength = static_cast<DWORD>(-1);
    Components.dwHostNameLength = static_cast<DWORD>(-1);
    Components.dwUrlPathLength = static_cast<DWORD>(-1);
    Components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wstrNormalized.c_str(), static_cast<DWORD>(wstrNormalized.length()), 0, &Components))
    {
        m_dwLastError = ::GetLastError();
        return false;
    }

    m_wstrHost.assign(Components.lpszHostName, Components.dwHostNameLength);

    if (Components.dwUrlPathLength > 0)
        m_wstrBasePath.assign(Components.lpszUrlPath, Components.dwUrlPathLength);
    else
        m_wstrBasePath.clear();

    if (!m_wstrBasePath.empty() && m_wstrBasePath.back() == L'/')
        m_wstrBasePath.pop_back();

    if (Components.nPort != 0)
        m_nPort = Components.nPort;
    else
        m_nPort = (Components.nScheme == INTERNET_SCHEME_HTTP) ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT;

    m_bUseTls = (Components.nScheme == INTERNET_SCHEME_HTTPS);
    m_wstrUserAgent = wstrUserAgent;

    if (!Reconnect())
    {
        CleanupUnlocked();
        return false;
    }

    m_dwLastError = ERROR_SUCCESS;
    return true;
}

bool CWinHttpRestClient::Get(const std::wstring& wstrPath, CWinHttpRestResponse& Response, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders)
{
    return SendRequest(L"GET", wstrPath, Response, vHeaders, nullptr, L"");
}

bool CWinHttpRestClient::Post(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders)
{
    return SendRequest(L"POST", wstrPath, Response, vHeaders, &strBody, wstrContentType);
}

bool CWinHttpRestClient::Put(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders)
{
    return SendRequest(L"PUT", wstrPath, Response, vHeaders, &strBody, wstrContentType);
}

bool CWinHttpRestClient::Patch(const std::wstring& wstrPath, const std::string& strBody, CWinHttpRestResponse& Response, const std::wstring& wstrContentType, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders)
{
    return SendRequest(L"PATCH", wstrPath, Response, vHeaders, &strBody, wstrContentType);
}

bool CWinHttpRestClient::Delete(const std::wstring& wstrPath, CWinHttpRestResponse& Response, const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders)
{
    return SendRequest(L"DELETE", wstrPath, Response, vHeaders, nullptr, L"");
}

void CWinHttpRestClient::SetTimeouts(const CWinHttpTimeouts& Timeouts)
{
    CCriticalSectionGuard Guard(m_csLock);
    m_Timeouts = Timeouts;
    m_dwLastError = ERROR_SUCCESS;
    ApplyTimeouts();
}

CWinHttpTimeouts CWinHttpRestClient::GetTimeouts() const
{
    CCriticalSectionGuard Guard(m_csLock);
    return m_Timeouts;
}

void CWinHttpRestClient::SetDefaultHeaders(std::vector<std::pair<std::wstring, std::wstring>> vHeaders)
{
    CCriticalSectionGuard Guard(m_csLock);
    m_vDefaultHeaders = std::move(vHeaders);
}

void CWinHttpRestClient::AddDefaultHeader(std::wstring wstrName, std::wstring wstrValue)
{
    CCriticalSectionGuard Guard(m_csLock);
    m_vDefaultHeaders.emplace_back(std::move(wstrName), std::move(wstrValue));
}

void CWinHttpRestClient::ClearDefaultHeaders()
{
    CCriticalSectionGuard Guard(m_csLock);
    m_vDefaultHeaders.clear();
}

DWORD CWinHttpRestClient::GetLastErrorCode() const
{
    CCriticalSectionGuard Guard(m_csLock);
    return m_dwLastError;
}

void CWinHttpRestClient::Cleanup()
{
    CCriticalSectionGuard Guard(m_csLock);
    CleanupUnlocked();
    m_dwLastError = ERROR_SUCCESS;
}

bool CWinHttpRestClient::EnsureConnection()
{
    if (m_hSession && m_hConnect)
        return true;

    return Reconnect();
}

bool CWinHttpRestClient::Reconnect()
{
    m_dwLastError = ERROR_SUCCESS;

    if (m_wstrUserAgent.empty() || m_wstrHost.empty())
    {
        m_dwLastError = ERROR_INVALID_PARAMETER;
        return false;
    }

    if (m_hSession)
    {
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
    }

    m_hSession = WinHttpOpen(m_wstrUserAgent.c_str(),
                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS,
                             0);
    if (!m_hSession)
    {
        m_dwLastError = ::GetLastError();
        return false;
    }

    ApplyTimeouts();
    if (m_dwLastError != ERROR_SUCCESS)
    {
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
        return false;
    }

    if (m_hConnect)
    {
        WinHttpCloseHandle(m_hConnect);
        m_hConnect = nullptr;
    }

    m_hConnect = WinHttpConnect(m_hSession, m_wstrHost.c_str(), m_nPort, 0);
    if (!m_hConnect)
    {
        m_dwLastError = ::GetLastError();
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
        return false;
    }

    return true;
}

std::wstring CWinHttpRestClient::BuildPath(const std::wstring& wstrPath) const
{
    if (wstrPath.empty())
        return m_wstrBasePath.empty() ? std::wstring(L"/") : m_wstrBasePath;

    if (!wstrPath.empty() && wstrPath.front() == L'/')
        return wstrPath;

    if (m_wstrBasePath.empty())
        return L"/" + wstrPath;

    std::wstring wstrCombined = m_wstrBasePath;
    wstrCombined.push_back(L'/');
    wstrCombined += wstrPath;
    return wstrCombined;
}

bool CWinHttpRestClient::SendRequest(const std::wstring& wstrMethod,
                                     const std::wstring& wstrPath,
                                     CWinHttpRestResponse& Response,
                                     const std::vector<std::pair<std::wstring, std::wstring>>& vHeaders,
                                     const std::string* pstrBody,
                                     const std::wstring& wstrContentType)
{
    CCriticalSectionGuard Guard(m_csLock);
    m_dwLastError = ERROR_SUCCESS;

    if (!EnsureConnection())
        return false;

    Response.Reset();

    std::wstring wstrRequestPath = BuildPath(wstrPath);
    DWORD dwFlags = m_bUseTls ? WINHTTP_FLAG_SECURE : 0;

    bool bRetryAttempted = false;
open_request:
    HINTERNET hRequest = WinHttpOpenRequest(m_hConnect,
                                            wstrMethod.c_str(),
                                            wstrRequestPath.c_str(),
                                            nullptr,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            dwFlags);
    if (!hRequest)
    {
        m_dwLastError = ::GetLastError();
        if (!bRetryAttempted && Reconnect())
        {
            bRetryAttempted = true;
            goto open_request;
        }
        return false;
    }

    std::vector<std::pair<std::wstring, std::wstring>> vCombinedHeaders = m_vDefaultHeaders;
    vCombinedHeaders.insert(vCombinedHeaders.end(), vHeaders.begin(), vHeaders.end());

    for (const auto& Header : vCombinedHeaders)
    {
        std::wstring wstrFullHeader = Header.first + L": " + Header.second + L"\r\n";
        WinHttpAddRequestHeaders(hRequest,
                                 wstrFullHeader.c_str(),
                                 static_cast<ULONG>(-1),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    if (pstrBody && !wstrContentType.empty())
    {
        std::wstring wstrContentTypeHeader = L"Content-Type: " + wstrContentType + L"\r\n";
        WinHttpAddRequestHeaders(hRequest,
                                 wstrContentTypeHeader.c_str(),
                                 static_cast<ULONG>(-1),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    DWORD dwTotalLength = pstrBody ? static_cast<DWORD>(pstrBody->size()) : 0;
    const void* pBodyData = (pstrBody && !pstrBody->empty()) ? pstrBody->data() : WINHTTP_NO_REQUEST_DATA;

    BOOL bSent = WinHttpSendRequest(hRequest,
                                    WINHTTP_NO_ADDITIONAL_HEADERS,
                                    0,
                                    const_cast<void*>(pBodyData),
                                    dwTotalLength,
                                    dwTotalLength,
                                    0);
    if (!bSent)
    {
        m_dwLastError = ::GetLastError();
        WinHttpCloseHandle(hRequest);
        if (!bRetryAttempted && Reconnect())
        {
            bRetryAttempted = true;
            goto open_request;
        }
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        m_dwLastError = ::GetLastError();
        WinHttpCloseHandle(hRequest);
        if (!bRetryAttempted && Reconnect())
        {
            bRetryAttempted = true;
            goto open_request;
        }
        return false;
    }

    DWORD dwStatusCode = 0;
    DWORD dwStatusCodeSize = sizeof(dwStatusCode);
    if (WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &dwStatusCode,
                            &dwStatusCodeSize,
                            WINHTTP_NO_HEADER_INDEX))
    {
        Response.SetStatusCode(dwStatusCode);
    }
    else
    {
        m_dwLastError = ::GetLastError();
        WinHttpCloseHandle(hRequest);
        return false;
    }

    DWORD dwHeaderSize = 0;
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        nullptr,
                        &dwHeaderSize,
                        WINHTTP_NO_HEADER_INDEX);

    DWORD dwHeaderQueryError = ::GetLastError();
    if (dwHeaderQueryError == ERROR_INSUFFICIENT_BUFFER && dwHeaderSize != 0)
    {
        std::wstring wstrHeadersBuffer;
        wstrHeadersBuffer.resize(dwHeaderSize / sizeof(wchar_t));
        if (WinHttpQueryHeaders(hRequest,
                                WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                &wstrHeadersBuffer[0],
                                &dwHeaderSize,
                                WINHTTP_NO_HEADER_INDEX))
        {
            Response.SetRawHeaders(wstrHeadersBuffer.c_str());
        }
    }

    std::string strResponseBody;
    DWORD dwAvailable = 0;
    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &dwAvailable))
        {
            m_dwLastError = ::GetLastError();
            WinHttpCloseHandle(hRequest);
            return false;
        }

        if (dwAvailable == 0)
            break;

        std::string strBuffer;
        strBuffer.resize(dwAvailable);
        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, &strBuffer[0], dwAvailable, &dwDownloaded))
        {
            m_dwLastError = ::GetLastError();
            WinHttpCloseHandle(hRequest);
            return false;
        }

        strBuffer.resize(dwDownloaded);
        strResponseBody.append(strBuffer);
    } while (dwAvailable > 0);

    Response.SetBody(std::move(strResponseBody));
    WinHttpCloseHandle(hRequest);
    m_dwLastError = ERROR_SUCCESS;

    return true;
}

void CWinHttpRestClient::ApplyTimeouts()
{
    if (!m_hSession)
        return;

    if (!WinHttpSetTimeouts(m_hSession, m_Timeouts.dwResolve, m_Timeouts.dwConnect, m_Timeouts.dwSend, m_Timeouts.dwReceive))
    {
        m_dwLastError = ::GetLastError();
    }
}

void CWinHttpRestClient::CleanupUnlocked()
{
    if (m_hConnect)
    {
        WinHttpCloseHandle(m_hConnect);
        m_hConnect = nullptr;
    }

    if (m_hSession)
    {
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
    }

    m_wstrHost.clear();
    m_wstrBasePath.clear();
    m_nPort = INTERNET_DEFAULT_HTTPS_PORT;
    m_bUseTls = true;
}
