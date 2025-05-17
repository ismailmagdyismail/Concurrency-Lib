#pragma once

#include <functional>
#include <thread>
#include <atomic>

class Thread
{
public:
    Thread(std::function<void(void)> &&p_oWorker);
    ~Thread();

    void Start();
    void Stop();

private:
    std::function<void(void)> m_oWorker;
    std::thread m_oThread;
    std::atomic<bool> m_bIsTerminated{false};
};