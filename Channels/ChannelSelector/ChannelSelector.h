#pragma once

//! System includes
#include <map>

//! Threading
#include <condition_variable>
#include <mutex>

//! Channels
#include "IChannel.h"

/*
    - Support multiple listeners on that same select channel
    - Support multiple diff Select statements at the same time
    - Support readers from one of the member channels directly, not through select statement
    - Support Diff Input channels sources with DIFF DATA TYPESSSSSS
*/

//! Questions / Edgecases:
//! Q: Why are We using TryReceive instead of Receive:
//!     since there may be other listners on those channels (through selectors or actual channels ) that have already read that value
//!     so we try to see if it still exists , cause if not and we used Recieve we will block forever ! (until the next value is ready )
//! Q: Why not just one bool for all channels to notify on
//!     to avoid lost notifications, what if a two channels notify
//! Q: Why not block till consumer (SelectAndExecute) ready that value ?
//!     we want the callback registerd to be Async, fire and forget in order to not impact other listeners on that channel
//! Q: Why are Handler for each Case statement is handled / executed while not Holding Lock ??:
//!     the handler is user defined code , holding lock while executing this user code has two risks
//!     1- if user's code involve heavy computation ? this will block all others for uncesseary time
//!     2- if user's code recalls some method from channel =>
//!             Deadlock (OR undefined behaviour according to C++ standard , double locking withing same thread)

class ChannelSelector
{
public:
    template <typename T>
    void AddChannel(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> &&p_fOnChannelDataAvailable)
    {
        std::lock_guard<std::mutex> oLock{m_oChannelsStateMutex};
        unsigned long long ullChanneldId = m_ullChannelId;

        m_oChannelsHandlers[ullChanneldId] = [p_pChannel, p_fOnChannelDataAvailable, ullChanneldId, this](std::function<void(void)> *p_fOutExecutionCallback) -> bool
        {
            return ReadAndDecorateHandler<T>(p_pChannel, p_fOnChannelDataAvailable, ullChanneldId, p_fOutExecutionCallback);
        };

        //! a copy of that pointer is kept, not ref since this will be executed in an async way (in the future)
        //! the HandleChannelInput is what is stored , not user passed callback ,
        //! We want the callback to be as light weight as possible and ALSO we want to execute user code in the consumers context
        //! HandleChannelInput just notifies any waiting threads (cosnumers) , and the user callback is executed in that context
        auto onDataAvailableHandler = std::bind(&ChannelSelector::HandleChannelInputReady, this, ullChanneldId);
        auto onChannelCloseHandler = std::bind(&ChannelSelector::HandleChannelClose, this, ullChanneldId);
        unsigned long long ullSelectorId = p_pChannel->RegisterChannelOperationsListener(onDataAvailableHandler, onChannelCloseHandler);
        m_oChannelsUnRegisterationHandlers[ullChanneldId] = {
            ullSelectorId,
            [ullSelectorId, p_pChannel]()
            { p_pChannel->UnRegisterChannelOperationsListener(ullSelectorId); }};
        m_ullChannelId++;
    }
    bool SelectAndExecute()
    {
        std::function<void(void)> fChannelHandler;
        {
            //! Muiltple Threads may be selecting from multiple channels , so this should be thread safe
            std::unique_lock<std::mutex> oLock{m_oChannelsStateMutex};

            //! Wait till on of the channels has data ready
            m_oChannelReadyCv.wait(oLock, [this]()
                                   { return m_bIsTerminated || AnyChannelReady() || isEmpty(); });

            if (m_bIsTerminated)
            {
                return false;
            }
            //! Consume Data
            bool bIsChannelReady = SelectAndReadFromAvailableChannel(&fChannelHandler);
            //! No channel was ready
            if (!bIsChannelReady)
            {
                return true;
            }
        }
        //! [IMP]
        //! Execute user code, without holding lock
        //! - So Other producers can put data without waiting on use code to finish
        //! - So if UserCode has loop or Sleep for example we don't need to block other producers from putting data on this channel
        //! - So we can Avoid Deadlocks if user tries calling other operations that hold that same Lock (double locking from same thread)
        fChannelHandler();
        return true;
    }
    void Close()
    {
        std::lock_guard<std::mutex> oLock{m_oChannelsStateMutex};
        m_bIsTerminated = true;
        m_oChannelReadyCv.notify_all();
        UnRegisterFromAllChannels();
    }

    ~ChannelSelector()
    {
        Close();
    }

private:
    template <typename T>
    bool ReadAndDecorateHandler(std::shared_ptr<IChannel<T>> p_pChannel, std::function<void(T &)> p_fOnChannelDataAvailable, unsigned long long p_ullChanneldId, std::function<void(void)> *p_fOutDecoratedHandler)
    {
        /*
        - Try Read value from the channel if availble
        - Created a callback with Read value passed as closure to be able to use it in a templated fashion and have type knowledge
        */
        T tVal;
        bool bResult = p_pChannel->TryReadValue(tVal);
        if (bResult)
        {
            //! C++ is stupid BTW
            *p_fOutDecoratedHandler = [p_fOnChannelDataAvailable, tVal]() mutable
            {
                p_fOnChannelDataAvailable(tVal);
            };
        }
        //! If No data available for that channel , then it must have been consumed by other threads in the mean time
        //! so mark it to 0 , I am holding the mutex anyway so any new data will increment counter after i exit this function
        else
        {
            m_oChannelsState[p_ullChanneldId] = 0;
        }
        return bResult;
    }
    bool SelectAndReadFromAvailableChannel(std::function<void(void)> *p_fOutDecoratedHandler)
    {
        for (auto &channelEntry : m_oChannelsState)
        {
            if (channelEntry.second <= 0)
            {
                continue;
            }
            auto handlerIt = m_oChannelsHandlers.find(channelEntry.first);
            bool readSuccess = handlerIt->second(p_fOutDecoratedHandler);
            channelEntry.second--;
            return readSuccess;
        }
        return false;
    }

    bool AnyChannelReady()
    {
        for (auto &entry : m_oChannelsState)
        {
            if (entry.second > 0)
            {
                return true;
            }
        }
        return false;
    }

    /*
        @breif callbacks for handling channels listeners operations
        - lightweight functions that just notify waiting threads
        - should be lightwieght in order not to block producers of data
        - they execute in the context of the producers on a certain channel , so we try to be as efficient as possible
    */
    void HandleChannelInputReady(unsigned long long p_ullChannelId)
    {
        //! as light weight as possible, to prevent blocking Producers that produced message
        std::lock_guard<std::mutex>
            oLock{m_oChannelsStateMutex};
        //! mark which channel was mark ready for faster checking
        m_oChannelsState[p_ullChannelId]++;
        //! Announce that some channel is ready
        m_oChannelReadyCv.notify_one();
    }
    void HandleChannelClose(unsigned long long p_ullChanneldId)
    {
        std::lock_guard<std::mutex> oLock{m_oChannelsStateMutex};
        m_oChannelsHandlers.erase(p_ullChanneldId);
        m_oChannelsState.erase(p_ullChanneldId);
    }

    void UnRegisterFromAllChannels()
    {

        for (auto &channelUnRegisterationHandlerEntry : m_oChannelsUnRegisterationHandlers)
        {
            channelUnRegisterationHandlerEntry.second.second();
        }
    }

    bool isEmpty() const
    {
        return m_oChannelsHandlers.size() == 0;
    }

    std::mutex m_oChannelsStateMutex;
    std::condition_variable m_oChannelReadyCv;
    unsigned long long m_ullChannelId = 0;
    std::map<unsigned long long, std::pair<unsigned long long, std::function<void(void)>>> m_oChannelsUnRegisterationHandlers;
    std::map<unsigned long long, unsigned long long> m_oChannelsState;
    std::map<unsigned long long, std::function<bool(std::function<void(void)> *)>> m_oChannelsHandlers;

    bool m_bIsTerminated{false};
};