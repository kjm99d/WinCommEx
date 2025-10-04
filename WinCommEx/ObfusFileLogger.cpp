#include "ObfusFileLogger.h"
#include <filesystem>
#include <random>
#include <cstdarg>

namespace fs = std::filesystem;

static std::vector<uint8_t> g_defaultKey = {
	0x6A,0x27,0xB3,0x51,0xC8,0x04,0x9F,0x21,
	0xD2,0xEE,0x10,0x8B,0x73,0xC1,0x5D,0x39,
	0x92,0xAE,0x44,0xF0,0x1C,0x67,0x2B,0x88,
	0x0D,0xBE,0x34,0x7A,0x55,0x11,0xE3,0x49
};

const std::vector<uint8_t>& CObfusFileLogger::DefaultKey() { return g_defaultKey; }

CObfusFileLogger::CObfusFileLogger(const std::wstring& logDir,
	const std::wstring& baseName,
	const std::vector<uint8_t>& key)
	: CFileLogger(logDir, baseName), m_key(key)
{
	// 부모의 EnsureFileOpened는 텍스트 .log를 열지만,
	// 우리는 별도의 .elog(바이너리)를 사용.
	// 필요하다면 .log는 열지 않아도 되므로 호출하지 않음.
	EnsureBinOpened();
}

CObfusFileLogger::~CObfusFileLogger()
{
	Close();
}

void CObfusFileLogger::SetKey(const std::vector<uint8_t>& key)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	m_key = key;
}

void CObfusFileLogger::EnsureBinOpened()
{
	std::lock_guard<std::mutex> lock(m_mtx);

	fs::create_directories(m_logDir);

	const std::wstring today = GetCurrentDateString();
	const bool needReopen = (today != m_currentDate) || !m_bin.is_open();

	if (needReopen) {
		if (m_bin.is_open()) m_bin.close();

		m_currentDate = today;
		// 텍스트 로그와 구분하려면 확장자를 바꾼다. (원하면 .log 그대로 써도 됨)
		m_logPath = MakeLogFilePath(m_currentDate); // 텍스트 경로(호환용)
		m_logPathBin = MakeBinPath();                  // 난독화 바이너리 경로

		// 바이너리 append
		m_bin.open(m_logPathBin, std::ios::binary | std::ios::app);
		if (m_bin.is_open()) {
			// 파일 시작 지점에 마커를 남기고 싶으면 여기에 쓰기 가능 (예: "WCE1")
			// 단, append 모드라면 첫 오픈 시점인지 확인이 필요할 수 있음.
		}
	}
}

std::wstring CObfusFileLogger::MakeBinPath() const
{
	// 예: C:\Logs\WinCommEx_2025-10-04.elog
	return m_logDir + L"\\" + m_baseName + L"_" + m_currentDate + L".elog";
}

std::string CObfusFileLogger::WStringToUtf8(const std::wstring& s)
{
	if (s.empty()) return {};
	int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
	std::string out(len, 0);
	WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), len, nullptr, nullptr);
	return out;
}

void CObfusFileLogger::XorObfuscate(std::vector<uint8_t>& data,
	const std::vector<uint8_t>& key,
	uint32_t salt)
{
	// 간단한 XOR 난독화: key를 순환하며 salt 로테이션
	const size_t klen = key.empty() ? 1 : key.size();
	for (size_t i = 0; i < data.size(); ++i) {
		uint8_t k = key[i % klen] ^ ((salt >> ((i % 4) * 8)) & 0xFF);
		data[i] ^= k;
	}
}

void CObfusFileLogger::WriteLine(const std::wstring& message)
{
	std::lock_guard<std::mutex> lock(m_mtx);

	EnsureBinOpened();
	if (!m_bin.is_open()) return;

	// 타임스탬프 + 메시지를 UTF-8로
	const std::wstring decorated = FormatTimePrefix() + message + L"\n";
	std::string utf8 = WStringToUtf8(decorated);
	std::vector<uint8_t> buf(utf8.begin(), utf8.end());

	// salt 생성
	std::random_device rd;
	std::mt19937 rng(rd());
	uint32_t salt = rng(); // 4-byte

	// XOR 난독화
	XorObfuscate(buf, m_key, salt);

	// [uint32 length][uint32 salt][cipher bytes]
	uint32_t len = static_cast<uint32_t>(buf.size());
	m_bin.write(reinterpret_cast<const char*>(&len), 4);
	m_bin.write(reinterpret_cast<const char*>(&salt), 4);
	if (len) m_bin.write(reinterpret_cast<const char*>(buf.data()), len);
	m_bin.flush();
}

void CObfusFileLogger::WriteFormat(const wchar_t* format, ...)
{
	wchar_t wbuf[1024];
	va_list args; va_start(args, format);
	_vsnwprintf_s(wbuf, _countof(wbuf), _TRUNCATE, format, args);
	va_end(args);
	WriteLine(wbuf);
}

void CObfusFileLogger::Close()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	if (m_bin.is_open()) {
		m_bin.flush();
		m_bin.close();
	}
	// 필요 시 텍스트 스트림도 닫음(부모가 가진 m_stream은 열지 않지만 안전차원)
	if (m_stream.is_open()) {
		m_stream.flush();
		m_stream.close();
	}
}
