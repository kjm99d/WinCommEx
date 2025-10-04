#pragma once
#include <Windows.h>

#include <string>

#include "Export.h"


//-----------------------------------------
// CMutex: Named or unnamed mutex wrapper
//-----------------------------------------
class WINCOMMEX_API CMutex {
public:
    // Constructor: if name provided, create named mutex
    explicit CMutex(const std::wstring& name = L"", bool bInitiallyOwned = false, LPSECURITY_ATTRIBUTES lpAttr = nullptr);
    ~CMutex();

    // Non-copyable
    CMutex(const CMutex&) = delete;
    CMutex& operator=(const CMutex&) = delete;

    // Movable
    CMutex(CMutex&& other) noexcept;
    CMutex& operator=(CMutex&& other) noexcept;

    // Wait until ownership
    DWORD Wait(DWORD dwMilliseconds = INFINITE) const;

    // Release ownership
    bool Release() const;

    // Get raw handle
    HANDLE GetHandle() const { return m_hMutex; }

    // Check valid
    bool IsValid() const { return m_hMutex && m_hMutex != INVALID_HANDLE_VALUE; }

private:
    HANDLE m_hMutex;
    std::wstring m_name;
};


/*

    CMutex g_mutex(L"Global\\WinCommEx_Mutex");

    {
        CScopedMutexLock lock(g_mutex, 2000);
        if (!lock.IsLocked()) {
            std::wcout << L"Already locked by another process\n";
            return 0;
        }

        std::wcout << L"Critical section acquired\n";
        Sleep(3000); // simulate work
    }

    std::wcout << L"Released mutex\n";


*/

/*

*/