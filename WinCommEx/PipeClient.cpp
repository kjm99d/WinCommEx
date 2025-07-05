#include "PipeClient.h"

CPipeClient::CPipeClient(const std::wstring& pipeName)
    : m_pipeName(pipeName), m_hPipe(INVALID_HANDLE_VALUE)
{
}

CPipeClient::~CPipeClient()
{
    Disconnect();
}

bool CPipeClient::Connect(DWORD timeoutMs)
{
    m_hPipe = CreateFileW(
        m_pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    return m_hPipe != INVALID_HANDLE_VALUE;
}

void CPipeClient::Disconnect()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
}

bool CPipeClient::Send(PPIPE_PROTOCOL req, PPIPE_PROTOCOL res)
{
    if (!SendPacket(req))
        return false;
    return ReceivePacket(res);
}

bool CPipeClient::SendPacket(PPIPE_PROTOCOL packet)
{
    DWORD totalSize = sizeof(PIPE_PROTOCOL) - 1 + packet->cbData;

    OVERLAPPED ov = {};
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) return false;

    ov.hEvent = hEvent;
    DWORD written = 0;

    BOOL result = WriteFile(m_hPipe, packet, totalSize, NULL, &ov);
    if (!result && GetLastError() == ERROR_IO_PENDING)
    {
        if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
            result = GetOverlappedResult(m_hPipe, &ov, &written, FALSE);
    }

    CloseHandle(hEvent);
    return result;
}

bool CPipeClient::ReceivePacket(PPIPE_PROTOCOL res, DWORD timeoutMs)
{
    DWORD headerSize = sizeof(PIPE_PROTOCOL) - 1;
    DWORD bytesRead = 0;

    OVERLAPPED ov = {};
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) return false;
    ov.hEvent = hEvent;

    BOOL result = ReadFile(m_hPipe, res, headerSize, NULL, &ov);
    if (!result && GetLastError() == ERROR_IO_PENDING)
    {
        if (WaitForSingleObject(hEvent, timeoutMs) == WAIT_OBJECT_0)
            result = GetOverlappedResult(m_hPipe, &ov, &bytesRead, FALSE);
        else {
            CancelIo(m_hPipe);
            CloseHandle(hEvent);
            return false;
        }
    }
    CloseHandle(hEvent);
    if (!result) return false;

    if (res->cbData > 0)
    {
        OVERLAPPED ov2 = {};
        HANDLE hEvent2 = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!hEvent2) return false;

        ov2.hEvent = hEvent2;
        result = ReadFile(m_hPipe, res->payload, res->cbData, NULL, &ov2);
        if (!result && GetLastError() == ERROR_IO_PENDING)
        {
            if (WaitForSingleObject(hEvent2, timeoutMs) == WAIT_OBJECT_0)
                result = GetOverlappedResult(m_hPipe, &ov2, &bytesRead, FALSE);
            else {
                CancelIo(m_hPipe);
                CloseHandle(hEvent2);
                return false;
            }
        }
        CloseHandle(hEvent2);
    }

    return result;
}

bool CPipeClient::Receive(void* buffer, DWORD bufferSize, DWORD& bytesRead, DWORD timeoutMs)
{
    bytesRead = 0;

    OVERLAPPED ov = {};
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) return false;

    ov.hEvent = hEvent;

    BOOL result = ReadFile(m_hPipe, buffer, bufferSize, NULL, &ov);
    if (!result && GetLastError() == ERROR_IO_PENDING)
    {
        DWORD wait = WaitForSingleObject(hEvent, timeoutMs);
        if (wait == WAIT_OBJECT_0)
        {
            result = GetOverlappedResult(m_hPipe, &ov, &bytesRead, FALSE);
        }
        else
        {
            CancelIo(m_hPipe);
            result = FALSE;
        }
    }

    CloseHandle(hEvent);
    return result;
}
