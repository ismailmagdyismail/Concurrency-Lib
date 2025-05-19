#include <iostream>
#include <thread>
#include <fstream>
#include <random>
#include <chrono>
#include <atomic>

#include "UnBufferedChannel.h"

class CounterActor
{
public:
    void Increment()
    {
        int delta = 1;
        m_oUpdateChannel.SendValue(std::move(delta));
    }

    void Decrement()
    {
        int delta = -1;
        m_oUpdateChannel.SendValue(std::move(delta));
    }

    void Stop()
    {
        m_bStopSignal = true;
        m_oUpdateChannel.Close();
    }

    void Run()
    {
        static std::ofstream logFile{"./counterActor.log.txt"};
        while (!m_bStopSignal)
        {
            //! Read and update value
            int delta;
            bool bReadValue = m_oUpdateChannel.ReadValue(delta);
            if (!bReadValue)
            {
                logFile << "Error in reading from channel" << std::endl;
                return;
            }
            m_iValue += delta;
            logFile << "Change: " << delta << " , newValue: " << m_iValue << std::endl;
        }
    }

private:
    UnBufferedChannel<int> m_oUpdateChannel;
    int m_iValue{0};
    std::atomic<bool> m_bStopSignal{false};
};

void incrementThread(CounterActor &p_oCounter)
{
    // static std::ofstream logFile{"./increment.log.txt"};
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<> dist(10, 100); // sleep time in ms

    for (int i = 0; i < 100; i++)
    {
        p_oCounter.Increment();
        // logFile << val << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
    }
}

void decrementThread(CounterActor &p_oCounter)
{
    // static std::ofstream logFile{"./decrement.log.txt"};
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<> dist(10, 100); // sleep time in ms

    for (int i = 0; i < 50; i++)
    {
        p_oCounter.Decrement();
        // logFile << val << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
    }
}

int main()
{
    CounterActor actor;
    std::thread t([&actor]()
                  { incrementThread(actor); });
    std::thread t2([&actor]()
                   { decrementThread(actor); });
    std::thread t3(std::bind(&CounterActor::Run, std::ref(actor)));
    // t3.detach();

    t.join();
    t2.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    actor.Stop();
    t3.join();
}