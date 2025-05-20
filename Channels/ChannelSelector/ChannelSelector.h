#pragma once

//! System includes

#include <map>

//! Threading
#include <condition_variable>
#include <mutex>

//! Channels
#include "IChannel.h"

/*
    - [Check] what if multiple channels notify at the same time
    - Support multiple listeners on that same select channel
    - Support multiple diff Select statements at the same time
    - Support readers from one of the member channels directly, not through select statement
*/

#include <iostream>

class ChannelSelector
{
public:
    template <typename T>
    void AddChannel(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> &&p_fOnChannelDataAvailable);
    bool SelectAndExecute();
    void Close();

    ~ChannelSelector();

private:
    template <typename T>
    void ExecuteSelectionCase(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> p_fOnChannelDataAvailable);
    bool SelectAndReadFromAvailableChannel(std::function<void(void)> &p_fHandler);
    void HandleChannelInputReady(unsigned long long p_ullChannelId);
    void HandleChannelClose(unsigned long long p_ullChanneldId);

    bool isEmpty() const;

    std::mutex m_oChannelMutex;
    std::condition_variable m_oChannelReadyCv;
    bool m_bChannelReady;
    unsigned long long m_ullChannelId = 0;
    std::map<unsigned long long, bool> m_oChannels;
    std::map<unsigned long long, std::function<void(void)>> m_oChannelsHandlers;

    bool m_bIsTerminated{false};
};

//! Questions / Edgecases:
//! Q: Why are We using TryReceive instead of Receive:
//!     since there may be other listners on those channels (through selectors or actual channels ) that have already read that value
//!     so we try to see if it still exists , cause if not and we used Recieve we will block forever ! (until the next value is ready )
//! Q: Why not just one bool for all channels to notify on
//!     to avoid lost notifications, what if a two channels notify
//! Q: Why not block till consumer (SelectAndExecute) ready that value ?
//!     we want the callback registerd to be Async, fire and forget in order to not impact other listeners on that channel

template <typename T>
void ChannelSelector::AddChannel(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> &&p_fOnChannelDataAvailable)
{
    std::lock_guard<std::mutex> oLock{m_oChannelMutex};
    unsigned long long ullChanneldId = m_ullChannelId;
    m_oChannelsHandlers[ullChanneldId] = [p_pChannel, p_fOnChannelDataAvailable, this]()
    {
        //! Closure to the Old Id which was capture at this point in time
        ExecuteSelectionCase<T>(p_pChannel, p_fOnChannelDataAvailable);
    };
    m_ullChannelId++;
    //! a copy of that pointer is kept, not ref since this will be executed in an async way (in the future)
    //! the HandleChannelInput is what is stored , not user passed callback ,
    //! We want the callback to be as light weight as possible and ALSO we want to execute user code in the consumers context
    //! HandleChannelInput just notifies any waiting threads (cosnumers) , and the user callback is executed in that context
    auto onDataAvailableHandler = std::bind(&ChannelSelector::HandleChannelInputReady, this, ullChanneldId);
    auto onChannelCloseHandler = std::bind(&ChannelSelector::HandleChannelClose, this, ullChanneldId);
    std::cerr << "ReGISTEING" << std::endl;
    p_pChannel->RegisterChannelOperationsListener(onDataAvailableHandler, onChannelCloseHandler);
}

void ChannelSelector::HandleChannelClose(unsigned long long p_ullChanneldId)
{
    std::lock_guard<std::mutex> oLock{m_oChannelMutex};
    m_oChannelsHandlers.erase(p_ullChanneldId);
    m_oChannels.erase(p_ullChanneldId);
}

void ChannelSelector::HandleChannelInputReady(unsigned long long p_ullChannelId)
{
    std::cerr << "Input ready" << std::endl;
    //! as light weight as possible, to prevent blocking Producers that produced message
    std::lock_guard<std::mutex>
        oLock{m_oChannelMutex};
    //! mark which channel was mark ready for faster checking
    m_oChannels[p_ullChannelId] = true;
    //! Announce that some channel is ready
    m_bChannelReady = true;
    m_oChannelReadyCv.notify_one();
    std::cerr << "Notify done" << std::endl;
}

template <typename T>
void ChannelSelector::ExecuteSelectionCase(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> p_fOnChannelDataAvailable)
{
    T tVal;
    bool bResult = p_pChannel->TryReadValue(tVal);
    if (bResult)
    {
        p_fOnChannelDataAvailable(tVal);
    }
}

bool ChannelSelector::SelectAndExecute()
{
    std::function<void(void)> fChannelHandler;
    {
        //! Muiltple Threads may be selecting from multiple channels , so this should be thread safe
        std::unique_lock<std::mutex> oLock{m_oChannelMutex};

        std::cerr << "TRYING TO SEKECT" << std::endl;

        //! Wait till on of the channels has data ready
        m_oChannelReadyCv.wait(oLock, [this]()
                               { return m_bIsTerminated || m_bChannelReady || isEmpty(); });

        std::cerr << "AWAKEN" << std::endl;
        if (isEmpty())
        {
            std::cerr << "EMPTY" << std::endl;
        }
        if (m_bIsTerminated)
        {
            return false;
        }
        //! Consume Data
        m_bChannelReady = false;
        bool bIsChannelReady = SelectAndReadFromAvailableChannel(fChannelHandler);
        //! No channel was ready
        if (!bIsChannelReady)
        {
            std::cerr << "NO CHANNELS READT" << std::endl;
            return true;
        }
    }
    std::cerr << "FOUND A CHANNELLLL" << std::endl;
    //! [IMP]
    //! Execute user code, without holding lock
    //! - So Other producers can put data without waiting on use code to finish
    //! - So if UserCode has loop or Sleep for example we don't need to block other producers from putting data on this channel
    //! - So we can Avoid Deadlocks if user tries calling other operations that hold that same Lock (double locking from same thread)
    fChannelHandler();
    return true;
}

bool ChannelSelector::SelectAndReadFromAvailableChannel(std::function<void(void)> &p_fHandler)
{
    for (auto &channelEntry : m_oChannels)
    {
        if (channelEntry.second)
        {
            auto handlerIt = m_oChannelsHandlers.find(channelEntry.first);
            p_fHandler = handlerIt->second;
            //! Consume value available
            //! Reset state , TryReading value from channel
            channelEntry.second = false;
            // p_fHandler = [this]() {

            // };
            return true;
        }
    }
    return false;
}

bool ChannelSelector::isEmpty() const
{
    return m_oChannelsHandlers.size() == 0;
}

void ChannelSelector::Close()
{
    std::lock_guard<std::mutex> oLock{m_oChannelMutex};
    m_bIsTerminated = true;
}

ChannelSelector::~ChannelSelector()
{
    Close();
}
