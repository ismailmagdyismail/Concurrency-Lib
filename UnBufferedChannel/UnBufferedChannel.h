#pragma once

#include <mutex>
#include <condition_variable>

template <typename T>
class UnBufferedChannel
{
public:
    //! Block till any previous values are Read
    //! this should block caller till some other thread read any previously store values
    //! this should be called by writer / producer thread
    bool SendValue(T &p_tValue)
    {
        std::unique_lock<std::mutex> lock{m_oMutex};

        //! Block until previous value is consumed
        m_oCv.wait(lock, [this]()
                   { return !m_bIsValueRecieved || m_bIsTerminationRequested; });

        if (m_bIsTerminationRequested)
        {
            return false;
        }

        //! Move value set by Writer / producer thread
        m_tRecievedValue = p_tValue;
        m_bIsValueRecieved = true;
        m_oCv.notify_one();

        //! Block until value Handled
        //! this is not necesscary , you can fire and forget ; since the following send call will block till consumption any way
        //! this just ensure that when send returns a reciever has 100% handled my request
        //! removing it is logically correct , and won't cause any race conditions
        //! this is More Like GO style of Channels
        //! Handles Backpressure, so that producers don't run so far ahead
        // m_oCv.wait(lock, [this]()
        //    { return !m_bIsValueRecieved || m_bIsTerminationRequested; });

        return !m_bIsTerminationRequested;
    }

    //! Block till value is available
    //! this should block caller till some other thread puts a value on the channel
    //! this should be called by reader / consumer thread
    bool ReadValue(T &p_tValue)
    {
        std::unique_lock<std::mutex> lock{m_oMutex};
        m_oCv.wait(lock, [this]()
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
        //! TEMP FIX
        m_oCv.notify_all();
        return true;
    }

    void Close()
    {
        std::lock_guard<std::mutex> lock{m_oMutex};
        m_bIsTerminationRequested = true;
        m_oCv.notify_all();
    }

private:
    void Reset()
    {
        m_bIsValueRecieved = false;
        m_bIsTerminationRequested = false;
    }

    //! Value received on the channel
    T m_tRecievedValue;
    bool m_bIsValueRecieved{false};
    bool m_bIsTerminationRequested{false};

    //! Synchronization
    std::condition_variable m_oCv;
    std::mutex m_oMutex;
};