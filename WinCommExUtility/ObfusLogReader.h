#pragma once
#include <Windows.h>
#include <string>
#include <fstream>
#include <vector>


//-------------------------------------------------------------
// CObfusLogReader : CObfusFileLogger�� ������ �α� ��ȣȭ Ŭ����
//-------------------------------------------------------------
class CObfusLogReader
{
public:
	explicit CObfusLogReader(const std::wstring& filePath,
		const std::vector<uint8_t>& key);
	~CObfusLogReader();

	// ������ ��ȿ�ϰ� ���ȴ��� Ȯ��
	bool IsOpen() const;

	// ���� �α� �� ���� ��ȣȭ�ؼ� ��ȯ (UTF-16)
	// EOF �� false ��ȯ
	bool ReadNext(std::wstring& outLine);

	// ��ü �α׸� �ѹ��� ��ȣȭ (���ǿ�)
	std::vector<std::wstring> ReadAll();

	// ���� ��ġ�� ���� ó������ �ǵ���
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
