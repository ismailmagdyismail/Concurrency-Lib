#pragma once

#include <functional>

#include "IChannel.h"

/*
- NOT USED
- Was Just an IDEA
*/

/*
- this is a wrapper/Decorator/proxy around IChannel (Buffered || unBuffered)
- its job is to Wrap , listen on certain events that happens within a channel and notify subscribers
- its mainely done to avoid changing the IChannel and all of its implementers for this , which is not the main responsibility or purpose for channels
- channels are there for async (buffered) or blocking (unbuffered) reading , writing not designed originally for listening
- I didn't want to change thier code for this , and for any upcoming behaviour ! they have 1 job any other extension can be done around them not in them directly
- Kinda like expression problem , Visitor , component systems ? => since we are adding more behaviour and such
- Also this is only created for the sake of Select Statement and multiplexing , it shoildn't be added to Channels were no one else needs IT!!
*/

/*
    - NOT USED NOW, just an idea
    - problems with it :
        - would require all already created channels to create a chennel of this type instead ! (create normal channel + pass it here)
        - Normally I would do this approach to avoid bloating out channel itself , but Select Should work with all kinds of channels so changing channels
            itself makes more sense here
*/

template <typename T>
class ChannelEventsListener
{
public:
    void RegisterDataAvailbleListener(std::function<void(void)> &&p_fOnDataAvailableListener)
    {
        m_fOnDataAvailableListener = std::move(p_fOnDataAvailableListener);
    }

    bool SendValue(T &&p_oValue)
    {
        bool bResult = m_pChannel->SendValue(p_oValue);
        if (bResult && m_fOnDataAvailableListener)
        {
            m_fOnDataAvailableListener();
        }
        return bResult;
    }

    bool ReadValue(T &p_oValue)
    {
        return m_pChannel->ReadValue(p_oValue);
    }

    void Close()
    {
        m_pChannel->Close();
    }

private:
    std::shared_ptr<IChannel<T>> m_pChannel;
    std::function<void(void)> m_fOnDataAvailableListener;
};