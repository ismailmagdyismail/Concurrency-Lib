#pragma once

#include "BasicThreadPool.h"
#include "UnBufferedChannel.h"

BasicThreadPool::BasicThreadPool()
{
    unsigned int uiThreadsCount = std::thread::hardware_concurrency();
    for (unsigned int uiThreadNumber = 0; uiThreadNumber < uiThreadsCount; ++uiThreadNumber)
    {
        m_vecWorkers.push_back(std::make_shared<Thread>());
        m_vecWorkers.back()->StartTask(std::bind(&BasicThreadPool::WorkerHandler, this));
    }
}

BasicThreadPool::~BasicThreadPool()
{
    Stop();
}

void BasicThreadPool::Stop()
{
    {
        std::lock_guard<std::mutex> oLock{m_oTasksMutex};
        m_bIsTerminated = true;
    }
    m_oTasksCv.notify_all();
}

template <typename T>
BasicThreadPool::ResultChannel<T> BasicThreadPool::SubmitTask(std::function<T(void)> &&p_fTask)
{
    ResultChannel<T> pResultChannel = std::make_shared<UnBufferedChannel<T>>();
    TaskWrapper fTaskWrapper = [pResultChannel, fTaskToExecute = std::move(p_fTask)]()
    {
        T tResult = fTaskToExecute();
        pResultChannel->SendValue(std::move(tResult));
    };

    {
        std::lock_guard<std::mutex> oLock{m_oTasksMutex};
        m_oTasks.push(fTaskWrapper);
    }
    m_oTasksCv.notify_one();
    return pResultChannel;
}

void BasicThreadPool::WorkerHandler()
{
    while (!m_bIsTerminated)
    {
        TaskWrapper fTask;
        {
            std::unique_lock<std::mutex> oLock{m_oTasksMutex};
            m_oTasksCv.wait(oLock, [this]()
                            { return m_bIsTerminated || !m_oTasks.empty(); });
            //! Only Stops if the signal Is Sent && All Tasks Are Consumed
            if (m_bIsTerminated && m_oTasks.empty())
            {
                return;
            }
            fTask = m_oTasks.front();
            m_oTasks.pop();
        }
        //! Execute Task after Releasing Lock
        //! Avoid Deadlocks | Undefined behaviour if the user code (Task) calls Thread Pool again
        //! Will try to lock while holding the lock from the same thread
        fTask();
    }
}