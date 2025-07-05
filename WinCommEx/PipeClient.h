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
    bool Send(PPIPE_PROTOCOL req, PPIPE_PROTOCOL res); // ����� Request/Response �Լ�
    bool Receive(void* buffer, DWORD bufferSize, DWORD& bytesRead, DWORD timeoutMs = INFINITE);
    void Disconnect();

private:
    bool SendPacket(PPIPE_PROTOCOL packet);         // ��û ���۸� ���
    bool ReceivePacket(PPIPE_PROTOCOL res, DWORD timeoutMs = 3000); // ���� ���Ÿ� ���

private:
    std::wstring m_pipeName;
    HANDLE m_hPipe;
};


/*


#include "PipeClient.h"
#include <iostream>

int main()
{
    // 1. ������ �̸� ����
    CPipeClient client(L"\\\\.\\pipe\\MyNamedPipe");

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
        std::wcout << L"[Client] ���� ����: " << std::wstring(reply.begin(), reply.end()) << std::endl;
    } else {
        std::wcerr << L"[Client] ��û ���� �Ǵ� ���� ���� ����" << std::endl;
    }

    // 6. �ڿ� ����
    free(req);
    free(res);
    client.Disconnect();
    return 0;
}


*/