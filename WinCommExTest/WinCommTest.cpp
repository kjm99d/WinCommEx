#include "PipeClient.h"
#include "PipeServer.h"

#include <thread>
#include <iostream>

#pragma comment(lib, "WinCommEx.lib")


std::wstring g_strPipeName = L"\\\\.\\pipe\\MyNamedPipe";

int threadFunction() {
    CPipeServer server(g_strPipeName);

    if (!server.Create()) {
        std::wcerr << L"[Server] ������ ���� ����" << std::endl;
        return 1;
    }

    if (!server.WaitForClient(5000)) {
        std::wcerr << L"[Server] Ŭ���̾�Ʈ ���� ����" << std::endl;
        return 1;
    }

    DWORD reqSize = sizeof(PIPE_PROTOCOL) - 1 + 512;
    PPIPE_PROTOCOL req = (PPIPE_PROTOCOL)malloc(reqSize);
    memset(req, 0, reqSize);

    if (server.Receive(req, 1000 * 10)) {
        std::string command((char*)req->payload, req->cbData);
        //std::wcout << L"[Server] ���� ��û: " << std::wstring(command.begin(), command.end()) << std::endl;
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


    // 1. ������ �̸� ����
    CPipeClient client(g_strPipeName);

    // 2. ���� ���� �õ�
    if (!client.Connect(3000)) {
        std::wcerr << L"[Client] ������ ���� ����" << std::endl;
        return 1;
    }

    // 3. ��û �޽��� �ۼ�
    std::string msg = "GetTime";
    DWORD reqSize = sizeof(PIPE_PROTOCOL) - 1 + (DWORD)msg.size();

    PPIPE_PROTOCOL req = (PPIPE_PROTOCOL)malloc(reqSize);
    req->cbData = (DWORD)msg.size();
    req->dwType = 1; // ��: GetTime ��û Ÿ��
    memcpy(req->payload, msg.data(), msg.size());

    // 4. ���� ���� �غ� (�ִ� 1024����Ʈ ���� ���)
    DWORD resSize = sizeof(PIPE_PROTOCOL) - 1 + 1024;
    PPIPE_PROTOCOL res = (PPIPE_PROTOCOL)malloc(resSize);
    memset(res, 0, resSize);

    // 5. ��û ���� �� ���� ����
    if (client.Send(req, res)) {
        std::string reply((char*)res->payload, res->cbData);
        //std::wcout << L"[Client] ���� ����: " << std::wstring(reply.begin(), reply.end()) << std::endl;
        MessageBoxW(NULL, std::wstring(reply.begin(), reply.end()).c_str(), NULL, MB_OK);
    }
    else {
        std::wcerr << L"[Client] ��û ���� �Ǵ� ���� ���� ����" << std::endl;
    }

    // 6. �ڿ� ����
    free(req);
    free(res);
    client.Disconnect();


	thread.join();

	return 0;
}