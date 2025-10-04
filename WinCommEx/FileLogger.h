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

	// ���� �޼���� ���� �� �Ļ�Ŭ�������� ����ȭ/��ȣȭ ���� ����
	virtual void WriteLine(const std::wstring& message);
	virtual void WriteFormat(const wchar_t* format, ...);

	std::wstring GetCurrentLogPath() const;

	// ����: �ʿ��ϸ� �ڽĿ��� �ݱ� ���� ���� ����
	virtual void Close();

protected:
	// �Ļ� Ŭ�������� ���� ���� ���� ����/��� ���� �� �ְ� protected�� ����
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
