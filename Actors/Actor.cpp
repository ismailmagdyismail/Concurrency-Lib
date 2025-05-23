#include "Actor.h"

Actor::Actor(std::function<void(void)> &&p_fEventLoopOperations)
    : m_fEventLoopOperations(std::move(p_fEventLoopOperations))
{
}

bool Actor::Start()
{
    std::lock_guard<std::mutex> oLock{m_oStatesMutex};
    if (m_bIsTerminated || m_bIsStarted)
    {
        return false;
    }
    m_bIsStarted = true;
    m_oActorThread.StartTask(std::bind(&Actor::EventLoop, this));
    return true;
}

bool Actor::Stop()
{
    std::lock_guard<std::mutex> oLock{m_oStatesMutex};
    if (m_bIsTerminated)
    {
        return false;
    }
    m_bIsStarted = false;
    m_bIsTerminated = true;
    m_oActorThread.Stop();
    return true;
}

bool Actor::Pause()
{
    std::lock_guard<std::mutex> oLock{m_oStatesMutex};
    if (m_bIsTerminated || !m_bIsStarted)
    {
        return false;
    }
    m_bIsStarted = true;
    return true;
}

void Actor::EventLoop()
{
    while (m_bIsStarted)
    {
        m_fEventLoopOperations();
    }
}

Actor::~Actor()
{
    Stop();
}