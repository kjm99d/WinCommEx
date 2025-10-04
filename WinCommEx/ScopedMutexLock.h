#pragma once
#include <Windows.h>
#include "Mutex.h"

//-------------------------------------------------------------
// CScopedMutexLock : RAII ��� Mutex �ڵ� ���� ����� Ŭ����
//-------------------------------------------------------------
class CScopedMutexLock {
public:
    explicit CScopedMutexLock(const CMutex& mtx, DWORD timeout = INFINITE);
    ~CScopedMutexLock();

    // ���� �Ұ�
    CScopedMutexLock(const CScopedMutexLock&) = delete;
    CScopedMutexLock& operator=(const CScopedMutexLock&) = delete;

    bool IsLocked() const { return m_locked; }

private:
    const CMutex& m_mutex;
    bool m_locked;
};