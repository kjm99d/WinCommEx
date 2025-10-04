#pragma once
#include "FileLogger.h"
#include <fstream>
#include <vector>


class CObfusFileLogger final : public CFileLogger
{
public:
	// key: ����ȭ Ű(���� ����Ʈ). �⺻���� ���ÿ�.
	explicit CObfusFileLogger(const std::wstring& logDir,
		const std::wstring& baseName = L"WinCommEx",
		const std::vector<uint8_t>& key = DefaultKey());

	~CObfusFileLogger() override;

	void WriteLine(const std::wstring& message) override;
	void WriteFormat(const wchar_t* format, ...) override;
	void Close() override;

	// Ű ��ü ����
	void SetKey(const std::vector<uint8_t>& key);

	// ���� ���� ���̴� ���̳ʸ� �α� ��� ��ȯ
	std::wstring GetEncryptedLogPath() const { return m_logPathBin; }

	// �⺻ Ű ����(����)
	static const std::vector<uint8_t>& DefaultKey();

protected:
	void EnsureBinOpened();               // ���̳ʸ� ���� ���� ����
	std::wstring MakeBinPath() const;     // .elog ���
	static std::string WStringToUtf8(const std::wstring& s);
	static void XorObfuscate(std::vector<uint8_t>& data,
		const std::vector<uint8_t>& key,
		uint32_t salt);

private:
	std::ofstream       m_bin;            // ���̳ʸ� ���(����ȭ�� payload ����)
	std::wstring        m_logPathBin;     // .elog ���
	std::vector<uint8_t> m_key;
};

