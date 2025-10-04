#pragma once
#include <Windows.h>
#include <string>
#include <fstream>
#include <vector>


//-------------------------------------------------------------
// CObfusLogReader : CObfusFileLogger로 생성된 로그 복호화 클래스
//-------------------------------------------------------------
class CObfusLogReader
{
public:
	explicit CObfusLogReader(const std::wstring& filePath,
		const std::vector<uint8_t>& key);
	~CObfusLogReader();

	// 파일이 유효하게 열렸는지 확인
	bool IsOpen() const;

	// 다음 로그 한 줄을 복호화해서 반환 (UTF-16)
	// EOF 시 false 반환
	bool ReadNext(std::wstring& outLine);

	// 전체 로그를 한번에 복호화 (편의용)
	std::vector<std::wstring> ReadAll();

	// 현재 위치를 파일 처음으로 되돌림
	void Reset();

private:
	static void XorObfuscate(std::vector<uint8_t>& data,
		const std::vector<uint8_t>& key,
		uint32_t salt);

	static std::wstring Utf8ToWString(const std::string& utf8);

private:
	std::wstring         m_path;
	std::ifstream        m_stream;
	std::vector<uint8_t> m_key;
};
