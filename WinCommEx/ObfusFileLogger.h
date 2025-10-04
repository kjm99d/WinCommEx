#pragma once
#include "FileLogger.h"
#include <fstream>
#include <vector>
#include "Export.h"

class WINCOMMEX_API CObfusFileLogger : public CFileLogger
{
public:
    explicit CObfusFileLogger(
        const std::wstring& logDir,
        const std::wstring& baseName = L"WinCommEx",
        const std::vector<uint8_t>& key = DefaultKey(),
        const std::wstring& postfix = L"");

    ~CObfusFileLogger() override;

    void WriteLine(const std::wstring& message) override;
    void WriteFormat(const wchar_t* format, ...) override;
    void Close() override;

    void SetKey(const std::vector<uint8_t>& key);
    std::wstring GetEncryptedLogPath() const { return m_logPathBin; }

    static const std::vector<uint8_t>& DefaultKey();

protected:
    void EnsureBinOpened();
    std::wstring MakeBinPath() const;
    static std::string WStringToUtf8(const std::wstring& s);
    static void XorObfuscate(std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        uint32_t salt);

private:
    std::ofstream        m_bin;
    std::wstring         m_logPathBin;
    std::vector<uint8_t> m_key;
    std::wstring         m_postfix;
};
