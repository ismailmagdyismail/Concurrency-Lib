#pragma once

#include <functional>

template <typename T>
class IChannel
{
public:
    virtual bool SendValue(T &&p_tValue) = 0;
    virtual bool SendValue(T &p_tValue) = 0;
    virtual bool ReadValue(T &p_tValue) = 0;
    virtual bool TryReadValue(T &p_tValue) = 0;
    virtual void Close() = 0;

    virtual ~IChannel() = default;

protected:
    //! This is kinda of a leaky abstract, I did it to support Multiplexing channels / Select statement
    //! could be made protected , Select/Multiplexer class marked as friend
    virtual unsigned long long RegisterChannelOperationsListener(
        std::function<void(void)> m_fOnDataAvailableCallback,
        std::function<void(void)> m_fOnCloseAvailableCallback) = 0;
    virtual void UnRegisterChannelOperationsListener(int) = 0;
    friend class ChannelSelector;
};