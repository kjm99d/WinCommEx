#pragma once
#include "FileLogger.h"
#include <fstream>
#include <vector>


class CObfusFileLogger final : public CFileLogger
{
public:
	// key: 난독화 키(임의 바이트). 기본값은 예시용.
	explicit CObfusFileLogger(const std::wstring& logDir,
		const std::wstring& baseName = L"WinCommEx",
		const std::vector<uint8_t>& key = DefaultKey());

	~CObfusFileLogger() override;

	void WriteLine(const std::wstring& message) override;
	void WriteFormat(const wchar_t* format, ...) override;
	void Close() override;

	// 키 교체 가능
	void SetKey(const std::vector<uint8_t>& key);

	// 현재 실제 쓰이는 바이너리 로그 경로 반환
	std::wstring GetEncryptedLogPath() const { return m_logPathBin; }

	// 기본 키 제공(샘플)
	static const std::vector<uint8_t>& DefaultKey();

protected:
	void EnsureBinOpened();               // 바이너리 파일 열기 보장
	std::wstring MakeBinPath() const;     // .elog 경로
	static std::string WStringToUtf8(const std::wstring& s);
	static void XorObfuscate(std::vector<uint8_t>& data,
		const std::vector<uint8_t>& key,
		uint32_t salt);

private:
	std::ofstream       m_bin;            // 바이너리 출력(난독화된 payload 저장)
	std::wstring        m_logPathBin;     // .elog 경로
	std::vector<uint8_t> m_key;
};

