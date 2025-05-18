#pragma once

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
    bool SendValue(T &p_tValue)
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
        m_tRecievedValue = p_tValue;
        m_bIsValueRecieved = true;
        m_oRecieveCv.notify_one();

        return true;
    }

    //! Block till value is available
    //! this should block caller till some other thread puts a value on the channel
    //! this should be called by reader / consumer thread
    bool ReadValue(T &p_tValue)
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
        m_oSendCv.notify_one();
        return true;
    }

    void Close()
    {
        std::lock_guard<std::mutex> lock{m_oMutex};
        m_bIsTerminationRequested = true;
        m_oRecieveCv.notify_all();
        m_oSendCv.notify_all();
    }

    ~UnBufferedChannel()
    {
        Close();
    }

private:
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
};