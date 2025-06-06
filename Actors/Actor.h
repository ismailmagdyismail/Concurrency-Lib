#pragma once

//! System includes
#include <mutex>
#include <functional>

#include "Thread.h"

class Actor
{
public:
    Actor(std::function<void(void)> &&p_fEventLoopOperations);
    bool Start();
    virtual bool Stop();
    bool Pause();
    virtual ~Actor();

private:
    void EventLoop();
    std::function<void(void)> m_fEventLoopOperations;

    Thread m_oActorThread;
    std::mutex m_oStatesMutex;
    bool m_bIsStarted{false};
    bool m_bIsTerminated{false};
};