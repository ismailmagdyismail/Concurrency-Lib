#pragma once

//! System includes
#include <functional>
#include <thread>

//! Channels
#include "UnBufferedChannel.h"

struct ThreadOperationMessage
{
    std::function<void(void)> m_fMessageHandler;
};

class Thread
{
public:
    Thread();
    ~Thread();

    void StartTask(std::function<void(void)> &&p_oWorker);
    void Stop();

private:
    void Run();

    UnBufferedChannel<ThreadOperationMessage> m_oChannel;
    std::function<void(void)> m_oTask;
    std::thread m_oThread;
    bool m_bIsTerminated{false};
};