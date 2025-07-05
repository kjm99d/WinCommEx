#pragma once
#include "Export.h"

#include "PipeProtocol.h"
#include <Windows.h>
#include <string>

class WINCOMMEX_API CPipeServer
{
public:
    CPipeServer(const std::wstring& pipeName);
    ~CPipeServer();

    bool Create();
    bool WaitForClient(DWORD timeoutMs = INFINITE);
    bool Receive(PPIPE_PROTOCOL req, DWORD timeoutMs = 3000);
    bool Send(PPIPE_PROTOCOL res);
    void Disconnect();

private:
    bool ReceivePacket(PPIPE_PROTOCOL req, DWORD timeoutMs);
    bool SendPacket(PPIPE_PROTOCOL packet);

private:
    std::wstring m_pipeName;
    HANDLE m_hPipe;
};

/*

#include "PipeServer.h"
#include <iostream>

int main()
{
    CPipeServer server(L"\\\\.\\pipe\\MyNamedPipe");

    if (!server.Create()) {
        std::wcerr << L"[Server] 파이프 생성 실패" << std::endl;
        return 1;
    }

    if (!server.WaitForClient(5000)) {
        std::wcerr << L"[Server] 클라이언트 연결 실패" << std::endl;
        return 1;
    }

    DWORD reqSize = sizeof(PIPE_PROTOCOL) - 1 + 512;
    PPIPE_PROTOCOL req = (PPIPE_PROTOCOL)malloc(reqSize);
    memset(req, 0, reqSize);

    if (server.Receive(req)) {
        std::string command((char*)req->payload, req->cbData);
        std::wcout << L"[Server] 수신 요청: " << std::wstring(command.begin(), command.end()) << std::endl;

        std::string response = (command == "GetTime") ? "2025-07-05 22:30:00" : "Unknown Command";

        DWORD resSize = sizeof(PIPE_PROTOCOL) - 1 + response.size();
        PPIPE_PROTOCOL res = (PPIPE_PROTOCOL)malloc(resSize);
        res->cbData = (DWORD)response.size();
        res->dwType = 100;
        memcpy(res->payload, response.data(), response.size());

        server.Send(res);
        free(res);
    }

    free(req);
    server.Disconnect();
    return 0;
}


*/