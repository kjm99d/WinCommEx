#include "PipeClient.h"
#include "PipeServer.h"

#include <thread>
#include <iostream>

#pragma comment(lib, "WinCommEx.lib")


std::wstring g_strPipeName = L"\\\\.\\pipe\\MyNamedPipe";

int threadFunction() {
    CPipeServer server(g_strPipeName);

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

    if (server.Receive(req, 1000 * 10)) {
        std::string command((char*)req->payload, req->cbData);
        //std::wcout << L"[Server] 수신 요청: " << std::wstring(command.begin(), command.end()) << std::endl;
        //MessageBoxW(NULL, std::wstring(command.begin(), command.end()).c_str(), NULL, MB_OK);

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

int main()
{
	std::thread thread(threadFunction); // Send
    

	Sleep(1000 * 3); // 


    // 1. 파이프 이름 지정
    CPipeClient client(g_strPipeName);

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
        //std::wcout << L"[Client] 응답 수신: " << std::wstring(reply.begin(), reply.end()) << std::endl;
        MessageBoxW(NULL, std::wstring(reply.begin(), reply.end()).c_str(), NULL, MB_OK);
    }
    else {
        std::wcerr << L"[Client] 요청 실패 또는 응답 수신 실패" << std::endl;
    }

    // 6. 자원 정리
    free(req);
    free(res);
    client.Disconnect();


	thread.join();

	return 0;
}