#pragma once

#include <queue>

//! Threading
#include <mutex>
#include <condition_variable>

//! Channel Interface
#include "IChannel.h"

template <typename T>
class BufferedChannel : public IChannel<T>
{
public:
    BufferedChannel(std::size_t p_sChannelMaxSize)
        : m_sChannelMaxSize(p_sChannelMaxSize)
    {
        if (m_sChannelMaxSize <= 0)
        {
            throw std::logic_error("Cannot Create a BufferedChannel With Len <= 0");
        }
    }

    bool SendValue(T &&p_tValue)
    {
        std::unique_lock<std::mutex> olock{m_oBufferMutex};

        //! Block till a slot is available in the buffer
        m_oSlotAvailableCv.wait(olock, [this]()
                                { return m_oBuffer.size() < m_sChannelMaxSize || m_bIsTerminated; });

        if (m_bIsTerminated)
        {
            return false;
        }
        m_oBuffer.push(std::move(p_tValue));
        //! Notify anyone waiting to read from the buffer that a value is available
        m_oRecieveCv.notify_one();
        return true;
    }

    bool ReadValue(T &p_tValue)
    {
        std::unique_lock<std::mutex> olock{m_oBufferMutex};
        //! Block till a value is available in the buffer, i.e Not Empty
        m_oRecieveCv.wait(olock, [this]()
                          { return !m_oBuffer.empty() || m_bIsTerminated; });
        if (m_bIsTerminated)
        {
            return false;
        }

        //! Consume value
        p_tValue = std::move(m_oBuffer.front());
        m_oBuffer.pop();

        //! Notify Prodcuers that a slot has become available
        m_oSlotAvailableCv.notify_one();
        return true;
    }

    void Close()
    {
        std::lock_guard<std::mutex> oLock{m_oBufferMutex};
        m_bIsTerminated = true;
        m_oSlotAvailableCv.notify_all();
        m_oRecieveCv.notify_all();
    }

    ~BufferedChannel()
    {
        Close();
    }

private:
    std::mutex m_oBufferMutex;
    std::condition_variable m_oRecieveCv;
    std::condition_variable m_oSlotAvailableCv;

    std::queue<T> m_oBuffer;
    std::size_t m_sChannelMaxSize;
    bool m_bIsTerminated{false};
};