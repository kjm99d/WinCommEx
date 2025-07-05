#pragma once
#include "Export.h"

#include "PipeProtocol.h"

#include <Windows.h>
#include <string>


class WINCOMMEX_API CPipeClient
{
public:
    CPipeClient(const std::wstring& pipeName);
    ~CPipeClient();

    bool Connect(DWORD timeoutMs = 5000);
    bool Send(PPIPE_PROTOCOL req, PPIPE_PROTOCOL res); // 고수준 Request/Response 함수
    bool Receive(void* buffer, DWORD bufferSize, DWORD& bytesRead, DWORD timeoutMs = INFINITE);
    void Disconnect();

private:
    bool SendPacket(PPIPE_PROTOCOL packet);         // 요청 전송만 담당
    bool ReceivePacket(PPIPE_PROTOCOL res, DWORD timeoutMs = 3000); // 응답 수신만 담당

private:
    std::wstring m_pipeName;
    HANDLE m_hPipe;
};


/*


#include "PipeClient.h"
#include <iostream>

int main()
{
    // 1. 파이프 이름 지정
    CPipeClient client(L"\\\\.\\pipe\\MyNamedPipe");

    // 2. 서버 연결 시도
    if (!client.Connect(3000)) {
        std::wcerr << L"[Client] 파이프 연결 실패" << std::endl;
        return 1;
    }

    // 3. 요청 메시지 작성
    std::string msg = "GetTime";
    DWORD reqSize = sizeof(PIPE_PROTOCOL) - 1 + (DWORD)msg.size();

    PPIPE_PROTOCOL req = (PPIPE_PROTOCOL)malloc(reqSize);
    req->cbData = (DWORD)msg.size();
    req->dwType = 1; // 예: GetTime 요청 타입
    memcpy(req->payload, msg.data(), msg.size());

    // 4. 응답 버퍼 준비 (최대 1024바이트 응답 허용)
    DWORD resSize = sizeof(PIPE_PROTOCOL) - 1 + 1024;
    PPIPE_PROTOCOL res = (PPIPE_PROTOCOL)malloc(resSize);
    memset(res, 0, resSize);

    // 5. 요청 전송 및 응답 수신
    if (client.Send(req, res)) {
        std::string reply((char*)res->payload, res->cbData);
        std::wcout << L"[Client] 응답 수신: " << std::wstring(reply.begin(), reply.end()) << std::endl;
    } else {
        std::wcerr << L"[Client] 요청 실패 또는 응답 수신 실패" << std::endl;
    }

    // 6. 자원 정리
    free(req);
    free(res);
    client.Disconnect();
    return 0;
}


*/