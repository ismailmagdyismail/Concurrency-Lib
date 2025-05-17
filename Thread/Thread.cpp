#include <iostream>

#include "Thread.h"

Thread::Thread(std::function<void(void)> &&p_oWorker)
    : m_oWorker(std::move(p_oWorker))
{
}

void Thread::Start()
{
    if (m_bIsTerminated || m_oThread.joinable())
    {
        std::cerr << "Thread is Used after termination" << std::endl;
        return;
        // throw std::logic_error("Thread is Used after termination");
    }
    std::function<void(void)> fWorker = [this]()
    {
        while (!m_bIsTerminated)
        {
            m_oWorker();
        }
    };
    m_oThread = std::thread(fWorker);
}

void Thread::Stop()
{
    m_bIsTerminated = true;
}

Thread::~Thread()
{
    if (m_oThread.joinable())
    {
        m_oThread.join();
    }
}