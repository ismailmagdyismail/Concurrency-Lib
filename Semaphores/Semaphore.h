#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
    Semaphore(int p_iCapacity) { m_iCapacity = p_iCapacity; }
    Semaphore(const Semaphore &) = delete;
    Semaphore(Semaphore &&) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    Semaphore &operator=(Semaphore &&) = delete;
    ~Semaphore() = default;

    /* to be used by lock_gaurd */
    bool lock()
    {
        return Accquire();
    }

    /* to be used by lock_gaurd */
    void unlock()
    {
        Release();
    }

    bool Accquire()
    {
        std::unique_lock<std::mutex> oLock{m_oCountMutex};
        m_oCountCv.wait(oLock, [this]()
                        { return m_iCapacity > 0 || m_bTerminated; });
        if (m_bTerminated)
        {
            return false;
        }
        m_iCapacity--;
        return true;
    }

    void Release()
    {
        std::lock_guard<std::mutex> oLock{m_oCountMutex};
        m_iCapacity++;
        m_oCountCv.notify_one();
    }

private:
    bool m_bTerminated{false};
    int m_iCapacity;
    std::mutex m_oCountMutex;
    std::condition_variable m_oCountCv;
};