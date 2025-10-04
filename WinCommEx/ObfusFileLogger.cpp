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
	// �θ��� EnsureFileOpened�� �ؽ�Ʈ .log�� ������,
	// �츮�� ������ .elog(���̳ʸ�)�� ���.
	// �ʿ��ϴٸ� .log�� ���� �ʾƵ� �ǹǷ� ȣ������ ����.
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
		// �ؽ�Ʈ �α׿� �����Ϸ��� Ȯ���ڸ� �ٲ۴�. (���ϸ� .log �״�� �ᵵ ��)
		m_logPath = MakeLogFilePath(m_currentDate); // �ؽ�Ʈ ���(ȣȯ��)
		m_logPathBin = MakeBinPath();                  // ����ȭ ���̳ʸ� ���

		// ���̳ʸ� append
		m_bin.open(m_logPathBin, std::ios::binary | std::ios::app);
		if (m_bin.is_open()) {
			// ���� ���� ������ ��Ŀ�� ����� ������ ���⿡ ���� ���� (��: "WCE1")
			// ��, append ����� ù ���� �������� Ȯ���� �ʿ��� �� ����.
		}
	}
}

std::wstring CObfusFileLogger::MakeBinPath() const
{
	// ��: C:\Logs\WinCommEx_2025-10-04.elog
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
	// ������ XOR ����ȭ: key�� ��ȯ�ϸ� salt �����̼�
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

	// Ÿ�ӽ����� + �޽����� UTF-8��
	const std::wstring decorated = FormatTimePrefix() + message + L"\n";
	std::string utf8 = WStringToUtf8(decorated);
	std::vector<uint8_t> buf(utf8.begin(), utf8.end());

	// salt ����
	std::random_device rd;
	std::mt19937 rng(rd());
	uint32_t salt = rng(); // 4-byte

	// XOR ����ȭ
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
	// �ʿ� �� �ؽ�Ʈ ��Ʈ���� ����(�θ� ���� m_stream�� ���� ������ ��������)
	if (m_stream.is_open()) {
		m_stream.flush();
		m_stream.close();
	}
}
