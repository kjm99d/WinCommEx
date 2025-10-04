#include "FileLogger.h"
#include <cstdarg>
#include <filesystem>

namespace fs = std::filesystem;

CFileLogger::CFileLogger(const std::wstring& logDir, const std::wstring& baseName)
	: m_logDir(logDir), m_baseName(baseName) {
}

CFileLogger::~CFileLogger() { Close(); }

void CFileLogger::EnsureFileOpened()
{
	// std::lock_guard<std::mutex> lock(m_mtx);

	fs::create_directories(m_logDir);

	const std::wstring today = GetCurrentDateString();
	const bool needReopen = (today != m_currentDate) || !m_stream.is_open();

	if (needReopen) {
		if (m_stream.is_open()) m_stream.close();

		m_currentDate = today;
		m_logPath = MakeLogFilePath(m_currentDate);

		m_stream.open(m_logPath, std::ios::app);
		if (m_stream.is_open()) {
			m_stream << L"========== Log Started: " << today << L" ==========\n";
			m_stream.flush();
		}
	}
}

std::wstring CFileLogger::GetCurrentDateString() const
{
	SYSTEMTIME st; GetLocalTime(&st);
	wchar_t buf[32]; swprintf_s(buf, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	return buf;
}

std::wstring CFileLogger::MakeLogFilePath(const std::wstring& date) const
{
	return m_logDir + L"\\" + m_baseName + L"_" + date + L".log";
}

std::wstring CFileLogger::FormatTimePrefix() const
{
	SYSTEMTIME st; GetLocalTime(&st);
	wchar_t buf[64];
	swprintf_s(buf, L"[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

void CFileLogger::WriteLine(const std::wstring& message)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	EnsureFileOpened();
	if (!m_stream.is_open()) return;

	m_stream << FormatTimePrefix() << message << L"\n";
	m_stream.flush();
}

void CFileLogger::WriteFormat(const wchar_t* format, ...)
{
	wchar_t buf[1024];
	va_list args; va_start(args, format);
	_vsnwprintf_s(buf, _countof(buf), _TRUNCATE, format, args);
	va_end(args);
	WriteLine(buf);
}

std::wstring CFileLogger::GetCurrentLogPath() const { return m_logPath; }

void CFileLogger::Close()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	if (m_stream.is_open()) {
		m_stream << L"========== Log Closed ==========\n";
		m_stream.flush();
		m_stream.close();
	}
}
