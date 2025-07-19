#pragma once

//! Tasks
#include <queue>
#include <functional>

//! Threading
#include <mutex>
#include <condition_variable>
#include <Thread.h>

template <typename T>
class UnBufferedChannel;

class BasicThreadPool
{
public:
    template <typename T>
    using ResultChannel = std::shared_ptr<UnBufferedChannel<T>>;
    using TaskWrapper = std::function<void(void)>;

    BasicThreadPool();
    ~BasicThreadPool();

    template <typename T>
    ResultChannel<T> SubmitTask(std::function<T(void)> &&p_fTask);
    void Stop();

private:
    void WorkerHandler();

    std::queue<TaskWrapper> m_oTasks;
    std::mutex m_oTasksMutex;
    std::condition_variable m_oTasksCv;
    bool m_bIsTerminated{false};
    std::vector<std::shared_ptr<Thread>> m_vecWorkers;
};

//! Template Implementaiton
#include "BasicThreadPool.cpp"