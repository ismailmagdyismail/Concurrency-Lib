#include <iostream>

#include "Thread.h"

Thread::Thread()
{
    m_oThread = std::thread(&Thread::Run, this);
}

void Thread::Run()
{
    while (!m_bIsTerminated)
    {
        ThreadOperationMessage oMessage;
        m_oChannel.ReadValue(oMessage);
        oMessage.m_fMessageHandler();
        m_oWorker = nullptr;
    }
}

void Thread::Start(std::function<void(void)> &&p_oWorker)
{
    ThreadOperationMessage oMessage;
    oMessage.m_fMessageHandler = [this, worker = std::move(p_oWorker)]()
    {
        m_oWorker = std::move(worker);
        m_oWorker();
    };
    m_oChannel.SendValue(oMessage);
}

void Thread::Stop()
{
    ThreadOperationMessage oMessage;
    oMessage.m_fMessageHandler = [this]()
    {
        m_bIsTerminated = true;
    };
    m_oChannel.SendValue(oMessage);
}

Thread::~Thread()
{
    Stop();
    if (m_oThread.joinable())
    {
        m_oThread.join();
    }
}