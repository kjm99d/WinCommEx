#include "ObfusLogReader.h"
#include <iostream>

int wmain()
{
    // 암호화 로그 경로 (예: C:\Logs\MyService_2025-10-04.elog)
    std::wstring path = L"C:\\Logs\\MyService_2025-10-04.elog";

    // 같은 키로 복호화해야 함 (CObfusFileLogger::DefaultKey 사용)
    // const auto& key = CObfusFileLogger::DefaultKey();
    const std::vector<uint8_t> vKey;

    CObfusLogReader reader(path, vKey);
    if (!reader.IsOpen()) {
        std::wcerr << L"로그 파일을 열 수 없습니다: " << path << std::endl;
        return 1;
    }

    std::wstring line;
    while (reader.ReadNext(line)) {
        std::wcout << line;
    }

    return 0;
}
