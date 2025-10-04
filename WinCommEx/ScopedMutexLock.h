#pragma once
#include <Windows.h>
#include "Mutex.h"

//-------------------------------------------------------------
// CScopedMutexLock : RAII 기반 Mutex 자동 해제 도우미 클래스
//-------------------------------------------------------------
class CScopedMutexLock {
public:
    explicit CScopedMutexLock(const CMutex& mtx, DWORD timeout = INFINITE);
    ~CScopedMutexLock();

    // 복사 불가
    CScopedMutexLock(const CScopedMutexLock&) = delete;
    CScopedMutexLock& operator=(const CScopedMutexLock&) = delete;

    bool IsLocked() const { return m_locked; }

private:
    const CMutex& m_mutex;
    bool m_locked;
};