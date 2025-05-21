#pragma once

//!
#include <map>

//! Threading
#include <mutex>
#include <condition_variable>

//! Channel Interface
#include "IChannel.h"

template <typename T>
class UnBufferedChannel : public IChannel<T>
{
public:
    //! Block till any previous values are Read
    //! this should block caller till some other thread read any previously store values
    //! this should be called by writer / producer thread
    virtual bool SendValue(T &&p_tValue) override
    {
        //! Lock is released after updating internal state
        //! This is important cause of the callback that we call (user defined code)
        //! What if that user code uses that same channel again to Send while we are holding mutex!!
        //! => Deadlock || Undefined behaviour cause of multiple locking within same thread
        //! So we release lock
        //! Also this minimized suprios wakeups on the Consumer threads
        {
            std::unique_lock<std::mutex> lock{m_oMutex};

            //! Block producers until previous value is consumed
            m_oSendCv.wait(lock, [this]()
                           { return !m_bIsValueRecieved || m_bIsTerminationRequested; });

            if (m_bIsTerminationRequested)
            {
                return false;
            }

            //! Move value set by Writer / producer thread
            m_tRecievedValue = std::move(p_tValue);
            m_bIsValueRecieved = true;
        }
        m_oRecieveCv.notify_one();

        //! Notify that Data Available
        //! this happens within the context of the thread that creates and puts it on the channel (Producer)
        //! Lock is release before calling it , to avoid deadlocks
        NotifyOnDataAvailableListeners();

        return true;
    }

    //! Block till value is available
    //! this should block caller till some other thread puts a value on the channel
    //! this should be called by reader / consumer thread
    virtual bool ReadValue(T &p_tValue) override
    {
        {
            std::unique_lock<std::mutex> lock{m_oMutex};
            //! Block consumers till a value is written
            m_oRecieveCv.wait(lock, [this]()
                              { return m_bIsValueRecieved || m_bIsTerminationRequested; });

            //! Channel was cleared
            //! Consume and reset
            if (m_bIsTerminationRequested)
            {
                Reset();
                return false;
            }

            //! Consume and reset
            p_tValue = m_tRecievedValue;
            Reset();
        }
        m_oSendCv.notify_one();
        return true;
    }

    virtual bool TryReadValue(T &p_tValue) override
    {
        {
            std::lock_guard<std::mutex> oLock{m_oMutex};
            if (m_bIsTerminationRequested || !m_bIsValueRecieved)
            {
                return false;
            }
            p_tValue = m_tRecievedValue;
            Reset();
        }
        m_oSendCv.notify_one();
        return true;
    }

    virtual void Close() override
    {
        {
            std::lock_guard<std::mutex> lock{m_oMutex};
            m_bIsTerminationRequested = true;
        }
        m_oRecieveCv.notify_all();
        m_oSendCv.notify_all();
        NotifyOnCloseListeners();
    }

    ~UnBufferedChannel()
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

    void Reset()
    {
        m_bIsValueRecieved = false;
        //! No Resetting to Termination Flag
        //! a Closed Channel cannot be reused For NOW
        // m_bIsTerminationRequested = false;
    }

    //! Value received on the channel
    T m_tRecievedValue;
    bool m_bIsValueRecieved{false};
    bool m_bIsTerminationRequested{false};

    //! Synchronization
    std::condition_variable m_oSendCv;    //! for producers
    std::condition_variable m_oRecieveCv; //! for consumers
    std::mutex m_oMutex;

    //! Listeners for Channel operations
    std::mutex m_oListenersMutex;
    std::map<int, std::function<void(void)>> m_oOnDataAvailableListeners;
    std::map<int, std::function<void(void)>> m_oOnCloseListeners;
    unsigned long long m_iID{0};
};