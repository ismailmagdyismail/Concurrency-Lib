#pragma once

template <typename T>
class IChannel
{
public:
    virtual bool SendValue(T &&p_tValue) = 0;
    virtual bool ReadValue(T &p_tValue) = 0;
    virtual void Close() = 0;

    virtual ~IChannel() = default;
};