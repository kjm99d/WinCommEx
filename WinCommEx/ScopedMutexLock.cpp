#include "ScopedMutexLock.h"

CScopedMutexLock::CScopedMutexLock(const CMutex& mtx, DWORD timeout)
    : m_mutex(mtx), m_locked(false)
{
    DWORD dwWait = m_mutex.Wait(timeout);
    m_locked = (dwWait == WAIT_OBJECT_0 || dwWait == WAIT_ABANDONED);
}

CScopedMutexLock::~CScopedMutexLock()
{
    if (m_locked)
        m_mutex.Release();
}