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
    void SendValue(T &p_tValue)
    {
        std::lock_guard<std::mutex> lock{m_oMutex};
        //! Move value set by Writer / producer thread
        m_tRecievedValue = p_tValue;
        m_bIsValueRecieved = true;
        m_oCv.notify_one();
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
        return true;
    }

    void Close()
    {
        std::lock_guard<std::mutex> lock{m_oMutex};
        m_bIsTerminationRequested = true;
        m_oCv.notify_one();
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