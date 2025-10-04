#include "ObfusLogReader.h"
#include <filesystem>
#include <string>
#include <sstream>

CObfusLogReader::CObfusLogReader(const std::wstring& filePath,
	const std::vector<uint8_t>& key)
	: m_path(filePath), m_key(key)
{
	m_stream.open(m_path, std::ios::binary);
}

CObfusLogReader::~CObfusLogReader()
{
	if (m_stream.is_open())
		m_stream.close();
}

bool CObfusLogReader::IsOpen() const
{
	return m_stream.is_open();
}

void CObfusLogReader::Reset()
{
	if (m_stream.is_open()) {
		m_stream.clear();
		m_stream.seekg(0, std::ios::beg);
	}
}

void CObfusLogReader::XorObfuscate(std::vector<uint8_t>& data,
	const std::vector<uint8_t>& key,
	uint32_t salt)
{
	const size_t klen = key.empty() ? 1 : key.size();
	for (size_t i = 0; i < data.size(); ++i) {
		uint8_t k = key[i % klen] ^ ((salt >> ((i % 4) * 8)) & 0xFF);
		data[i] ^= k;
	}
}

std::wstring CObfusLogReader::Utf8ToWString(const std::string& utf8)
{
	if (utf8.empty()) return {};
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
		static_cast<int>(utf8.size()), nullptr, 0);
	std::wstring out(len, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
		static_cast<int>(utf8.size()), out.data(), len);
	return out;
}

bool CObfusLogReader::ReadNext(std::wstring& outLine)
{
	outLine.clear();
	if (!m_stream.is_open() || m_stream.eof())
		return false;

	uint32_t len = 0;
	uint32_t salt = 0;

	m_stream.read(reinterpret_cast<char*>(&len), 4);
	if (m_stream.eof() || len == 0)
		return false;

	m_stream.read(reinterpret_cast<char*>(&salt), 4);
	if (m_stream.eof())
		return false;

	std::vector<uint8_t> buf(len);
	m_stream.read(reinterpret_cast<char*>(buf.data()), len);
	if (m_stream.gcount() != static_cast<std::streamsize>(len))
		return false;

	// XOR º¹È£È­
	XorObfuscate(buf, m_key, salt);

	// UTF-8 ¡æ UTF-16
	std::string utf8(buf.begin(), buf.end());
	outLine = Utf8ToWString(utf8);

	return true;
}

std::vector<std::wstring> CObfusLogReader::ReadAll()
{
	std::vector<std::wstring> lines;
	if (!m_stream.is_open())
		return lines;

	Reset();

	std::wstring line;
	while (ReadNext(line))
		lines.push_back(line);

	return lines;
}