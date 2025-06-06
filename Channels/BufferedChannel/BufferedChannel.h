#pragma once

#include <map>
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

    //! Block till any previous values are Read
    //! this should block caller till some other thread read any previously store values
    //! this should be called by writer / producer thread
    virtual bool SendValue(T &&p_tValue) override
    {
        constexpr bool bMove = true;
        return SendValue(p_tValue, bMove);
    }

    virtual bool SendValue(T &p_tValue) override
    {
        constexpr bool bMove = false;
        return SendValue(p_tValue, bMove);
    }

    bool SendValue(T &p_tValue, bool p_bMove)
    {
        //! Lock is released after updating internal state
        //! This is important cause of the callback that we call (user defined code)
        //! What if that user code uses that same channel again to Send while we are holding mutex!!
        //! => Deadlock || Undefined behaviour cause of multiple locking within same thread
        //! So we release lock
        //! Also this minimized suprios wakeups on the Consumer threads
        {
            std::unique_lock<std::mutex> olock{m_oBufferMutex};
            //! Block till a slot is available in the buffer
            m_oSlotAvailableCv.wait(olock, [this]()
                                    { return m_oBuffer.size() < m_sChannelMaxSize || m_bIsTerminated; });

            if (m_bIsTerminated)
            {
                return false;
            }
            if (p_bMove)
            {
                //! Move
                m_oBuffer.push(std::move(p_tValue));
            }
            else
            {
                //! Copy
                m_oBuffer.push(p_tValue);
            }
        }

        //! Notify anyone waiting to read from the buffer that a value is available
        m_oRecieveCv.notify_one();

        //! this happens within the context of the thread that creates and puts it on the channel (Producer)
        //! Lock is release before calling it , to avoid deadlocks
        NotifyOnDataAvailableListeners();

        return true;
    }

    virtual bool ReadValue(T &p_tValue) override
    {
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
        }
        //! Notify Prodcuers that a slot has become available
        m_oSlotAvailableCv.notify_one();
        return true;
    }

    virtual bool TryReadValue(T &p_tValue) override
    {
        {
            std::lock_guard<std::mutex> oLock{m_oBufferMutex};
            if (m_bIsTerminated || m_oBuffer.empty())
            {
                return false;
            }
            //! Consume value
            p_tValue = std::move(m_oBuffer.front());
            m_oBuffer.pop();
        }
        m_oSlotAvailableCv.notify_one();
        return true;
    }

    virtual void Close() override
    {
        {
            std::lock_guard<std::mutex> oLock{m_oBufferMutex};
            m_bIsTerminated = true;
        }
        m_oSlotAvailableCv.notify_all();
        m_oRecieveCv.notify_all();
        NotifyOnCloseListeners();
    }

    ~BufferedChannel()
    {
        Close();
    }

protected:
    virtual unsigned long long RegisterChannelOperationsListener(
        std::function<void(void)> p_fOnDataAvailableCallback,
        std::function<void(void)> p_fOnCloseAvailableCallback) override
    {
        std::lock_guard<std::mutex> oLock{m_oListenersMutex};
        unsigned long long iIdToUse = m_iID;
        m_oOnDataAvailableListeners[iIdToUse] = std::move(p_fOnDataAvailableCallback);
        m_oOnCloseListeners[iIdToUse] = std::move(p_fOnCloseAvailableCallback);
        m_iID++;
        return iIdToUse;
    }

    virtual void UnRegisterChannelOperationsListener(int p_iId) override
    {
        m_oOnDataAvailableListeners.erase(p_iId);
        m_oOnCloseListeners.erase(p_iId);
    }

private:
    void NotifyOnDataAvailableListeners()
    {
        std::lock_guard<std::mutex> oLock{m_oListenersMutex};
        for (auto &entry : m_oOnDataAvailableListeners)
        {
            entry.second();
        }
    }
    void NotifyOnCloseListeners()
    {
        std::lock_guard<std::mutex> oLock{m_oListenersMutex};
        for (auto &entry : m_oOnCloseListeners)
        {
            entry.second();
        }
    }

    std::mutex m_oBufferMutex;
    std::condition_variable m_oRecieveCv;
    std::condition_variable m_oSlotAvailableCv;

    std::queue<T> m_oBuffer;
    std::size_t m_sChannelMaxSize;
    bool m_bIsTerminated{false};

    //! Listeners for Channel operations
    std::mutex m_oListenersMutex;
    std::map<int, std::function<void(void)>> m_oOnDataAvailableListeners;
    std::map<int, std::function<void(void)>> m_oOnCloseListeners;
    unsigned long long m_iID{0};
};