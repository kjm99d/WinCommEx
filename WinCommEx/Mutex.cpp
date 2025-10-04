#include "Mutex.h"

CMutex::CMutex(const std::wstring& name, bool bInitiallyOwned, LPSECURITY_ATTRIBUTES lpAttr)
    : m_name(name), m_hMutex(nullptr)
{
    m_hMutex = CreateMutexW(lpAttr, bInitiallyOwned, name.empty() ? nullptr : name.c_str());
}

CMutex::~CMutex() {
    if (m_hMutex && m_hMutex != INVALID_HANDLE_VALUE)
        CloseHandle(m_hMutex);
}

CMutex::CMutex(CMutex&& other) noexcept
    : m_hMutex(other.m_hMutex), m_name(std::move(other.m_name))
{
    other.m_hMutex = nullptr;
}

CMutex& CMutex::operator=(CMutex&& other) noexcept {
    if (this != &other) {
        if (m_hMutex && m_hMutex != INVALID_HANDLE_VALUE)
            CloseHandle(m_hMutex);
        m_hMutex = other.m_hMutex;
        m_name = std::move(other.m_name);
        other.m_hMutex = nullptr;
    }
    return *this;
}

DWORD CMutex::Wait(DWORD dwMilliseconds) const {
    if (!IsValid())
        return WAIT_FAILED;
    return WaitForSingleObject(m_hMutex, dwMilliseconds);
}

bool CMutex::Release() const {
    if (!IsValid())
        return false;
    return ReleaseMutex(m_hMutex);
}