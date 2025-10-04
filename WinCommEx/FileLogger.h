#pragma once
#include <Windows.h>
#include <string>
#include <mutex>
#include <fstream>


class CFileLogger
{
public:
	explicit CFileLogger(const std::wstring& logDir, const std::wstring& baseName = L"WinCommEx");
	virtual ~CFileLogger();

	// 가상 메서드로 변경 → 파생클래스에서 난독화/암호화 구현 가능
	virtual void WriteLine(const std::wstring& message);
	virtual void WriteFormat(const wchar_t* format, ...);

	std::wstring GetCurrentLogPath() const;

	// 가상: 필요하면 자식에서 닫기 동작 변경 가능
	virtual void Close();

protected:
	// 파생 클래스에서 파일 오픈 상태 보장/경로 얻을 수 있게 protected로 공개
	void EnsureFileOpened();
	std::wstring MakeLogFilePath(const std::wstring& date) const;
	std::wstring GetCurrentDateString() const;
	std::wstring FormatTimePrefix() const;

protected:
	std::wstring m_logDir;
	std::wstring m_baseName;
	std::wstring m_logPath;
	std::wofstream m_stream;
	std::wstring m_currentDate;
	mutable std::mutex m_mtx;
};
