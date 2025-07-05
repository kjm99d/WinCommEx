#include "PipeServer.h"

CPipeServer::CPipeServer(const std::wstring& pipeName)
    : m_pipeName(pipeName), m_hPipe(INVALID_HANDLE_VALUE)
{
}

CPipeServer::~CPipeServer()
{
    Disconnect();
}

bool CPipeServer::Create()
{
    m_hPipe = CreateNamedPipeW(
        m_pipeName.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 4096, 4096, 0, NULL
    );

    return m_hPipe != INVALID_HANDLE_VALUE;
}

bool CPipeServer::WaitForClient(DWORD timeoutMs)
{
    OVERLAPPED ov = {};
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) return false;

    ov.hEvent = hEvent;

    BOOL result = ConnectNamedPipe(m_hPipe, &ov);
    if (!result && GetLastError() == ERROR_IO_PENDING)
    {
        DWORD wait = WaitForSingleObject(hEvent, timeoutMs);
        CloseHandle(hEvent);
        return wait == WAIT_OBJECT_0;
    }

    CloseHandle(hEvent);

    if (!result && GetLastError() == ERROR_PIPE_CONNECTED)
        return true;

    return result;
}

void CPipeServer::Disconnect()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(m_hPipe);
        DisconnectNamedPipe(m_hPipe);
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
}

bool CPipeServer::Receive(PPIPE_PROTOCOL req, DWORD timeoutMs)
{
    return ReceivePacket(req, timeoutMs);
}

bool CPipeServer::Send(PPIPE_PROTOCOL res)
{
    return SendPacket(res);
}

bool CPipeServer::ReceivePacket(PPIPE_PROTOCOL req, DWORD timeoutMs)
{
    DWORD headerSize = sizeof(PIPE_PROTOCOL) - 1;
    DWORD bytesRead = 0;

    OVERLAPPED ov = {};
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) return false;
    ov.hEvent = hEvent;

    BOOL result = ReadFile(m_hPipe, req, headerSize, NULL, &ov);
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

    if (req->cbData > 0)
    {
        OVERLAPPED ov2 = {};
        HANDLE hEvent2 = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!hEvent2) return false;

        ov2.hEvent = hEvent2;
        result = ReadFile(m_hPipe, req->payload, req->cbData, NULL, &ov2);
        if (!result && GetLastError() == ERROR_IO_PENDING) {
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

bool CPipeServer::SendPacket(PPIPE_PROTOCOL packet)
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
