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
        bool bResult = m_oChannel.ReadValue(oMessage);
        //! If No Result where read (Channel may have been terminated)
        if (!bResult)
        {
            continue;
        }
        oMessage.m_fMessageHandler();
    }
}

void Thread::StartTask(std::function<void(void)> &&p_oWorker)
{
    ThreadOperationMessage oMessage;
    oMessage.m_fMessageHandler = [this, worker = std::move(p_oWorker)]()
    {
        m_oTask = std::move(worker);
        m_oTask();
        m_oTask = nullptr;
    };
    m_oChannel.SendValue(oMessage);
}

void Thread::Stop()
{
    ThreadOperationMessage oMessage;
    oMessage.m_fMessageHandler = [this]()
    {
        //! Set Termination Signal to stop thread
        m_bIsTerminated = true;
        //! Close Communication Channel
        //! Any future Stop Requests will have any effect since channel is closed
        //! And closed channels cannot be reused
        m_oChannel.Close();
    };
    m_oChannel.SendValue(oMessage);
}

Thread::~Thread()
{
    if (m_oThread.joinable())
    {
        Stop();
        m_oThread.join();
    }
}