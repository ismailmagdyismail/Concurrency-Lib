#include "ChannelSelector.h"
#include "BufferedChannel.h"
#include "UnBufferedChannel.h"

#include <thread>
#include <iostream>
#include <chrono>
#include <fstream>

bool BUFFERED = true;

class SingleProducerScenario
{
public:
    std::ofstream logFile{"SingleProducer.log.txt"};
    void Producer()
    {
        for (int i = 0; i < 100; ++i)
        {
            channel->SendValue("MEssage from producer thread");
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel = std::make_shared<BufferedChannel<std::string>>(100);
        }
        else
        {
            channel = std::make_shared<UnBufferedChannel<std::string>>();
        }
        selector.AddChannel<std::string>(channel, [this](std::string &channelInput)
                                         { logFile << "Consumer Read value: " << channelInput << std::endl; });

        std::thread p1{&SingleProducerScenario::Producer, this};
        std::thread c{&SingleProducerScenario::Consumer, this};

        p1.join();
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        selector.Close();
        c.join();
    }

private:
    ChannelSelector selector;
    std::shared_ptr<IChannel<std::string>> channel;
};

class NProducerSameChannelScenario
{
public:
    std::ofstream logFile{"NProducerSameChannelScenario.log.txt"};
    void Producer(int n, int id)
    {
        for (int i = 0; i < n; ++i)
        {
            channel->SendValue("MEssage from producer thread with ID:" + std::to_string(id));
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel = std::make_shared<BufferedChannel<std::string>>(100);
        }
        else
        {
            channel = std::make_shared<UnBufferedChannel<std::string>>();
        }

        selector.AddChannel<std::string>(channel, [this](std::string &channelInput)
                                         { logFile << "Consumer Read value: " << channelInput << std::endl; });

        std::vector<std::thread> producers;
        std::thread c{&NProducerSameChannelScenario::Consumer, this};
        for (int i = 0; i < 5; i++)
        {
            producers.push_back(std::thread([this, i]()
                                            { Producer(20, i); }));
        }

        for (std::size_t i = 0; i < producers.size(); i++)
        {
            if (producers[i].joinable())
            {
                producers[i].join();
            }
            else
            {
                std::cerr << "producer  " << i << " Not joinable" << std::endl;
            }
        }
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        std::cerr << "ALl producers Finished,  Main thread About to Close Select....\n";
        selector.Close();
        std::cerr << "Main Closed it\n";

        c.join();
    }

private:
    ChannelSelector selector;
    std::shared_ptr<IChannel<std::string>> channel;
};

class NProducerDiffChannelsScenario
{
public:
    std::ofstream logFile{"NProducerDiffChannelsScenario.log.txt"};
    void Producer(std::shared_ptr<IChannel<std::string>> p_pChannel, int n, int id)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue("MEssage from producer thread with ID:" + std::to_string(id));
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel1 = std::make_shared<BufferedChannel<std::string>>(100);
            channel2 = std::make_shared<BufferedChannel<std::string>>(100);
            channel3 = std::make_shared<BufferedChannel<std::string>>(100);
        }
        else
        {
            channel1 = std::make_shared<UnBufferedChannel<std::string>>();
            channel2 = std::make_shared<UnBufferedChannel<std::string>>();
            channel3 = std::make_shared<UnBufferedChannel<std::string>>();
        }

        selector.AddChannel<std::string>(channel1, [this](std::string &channelInput)
                                         { logFile << "Consumer Read value From Channel 1: " << channelInput << std::endl; });
        selector.AddChannel<std::string>(channel2, [this](std::string &channelInput)
                                         { logFile << "Consumer Read value From Channel 2: " << channelInput << std::endl; });
        selector.AddChannel<std::string>(channel3, [this](std::string &channelInput)
                                         { logFile << "Consumer Read value From Channel 3: " << channelInput << std::endl; });

        std::thread c{&NProducerDiffChannelsScenario::Consumer, this};

        std::vector<std::thread> producers;
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 30, 1); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 10, 2); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 10, 3); }));

        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 20, 4); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 30, 5); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 10, 6); }));

        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 7); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 8); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 9); }));

        for (std::size_t i = 0; i < producers.size(); i++)
        {
            if (producers[i].joinable())
            {
                producers[i].join();
            }
            else
            {
                std::cerr << "producer  " << i << " Not joinable" << std::endl;
            }
        }
        std::cerr << "ALl producers Finished,  Main thread About to Close Select....\n";
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        selector.Close();
        std::cerr << "Main Closed it\n";

        c.join();
    }

private:
    ChannelSelector selector;
    std::shared_ptr<IChannel<std::string>> channel1;
    std::shared_ptr<IChannel<std::string>> channel2;
    std::shared_ptr<IChannel<std::string>> channel3;
};

class NProducerDiffToNSelectionConsumersChannelsScenario
{
public:
    std::ofstream logFile{"NProducerDiffToNSelectionConsumersChannelsScenario.log.txt"};

    void Producer(std::shared_ptr<IChannel<std::string>> p_pChannel, int n, int id)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue("MEssage from producer thread with ID:" + std::to_string(id));
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel1 = std::make_shared<BufferedChannel<std::string>>(100);
            channel2 = std::make_shared<BufferedChannel<std::string>>(100);
            channel3 = std::make_shared<BufferedChannel<std::string>>(100);
        }
        else
        {
            channel1 = std::make_shared<UnBufferedChannel<std::string>>();
            channel2 = std::make_shared<UnBufferedChannel<std::string>>();
            channel3 = std::make_shared<UnBufferedChannel<std::string>>();
        }

        std::mutex mx;

        selector.AddChannel<std::string>(channel1, [this, &mx](std::string &channelInput)
                                         { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From Channel 1: " << channelInput << std::endl; });
        selector.AddChannel<std::string>(channel2, [this, &mx](std::string &channelInput)
                                         { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From Channel 2: " << channelInput << std::endl; });
        selector.AddChannel<std::string>(channel3, [this, &mx](std::string &channelInput)
                                         { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From Channel 3: " << channelInput << std::endl; });

        std::thread c1{&NProducerDiffToNSelectionConsumersChannelsScenario::Consumer, this};
        std::thread c2{&NProducerDiffToNSelectionConsumersChannelsScenario::Consumer, this};
        std::thread c3{&NProducerDiffToNSelectionConsumersChannelsScenario::Consumer, this};

        std::vector<std::thread> producers;
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 30, 1); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 10, 2); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel1, 10, 3); }));

        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 20, 4); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 30, 5); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel2, 10, 6); }));

        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 7); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 8); }));
        producers.push_back(std::thread([this]()
                                        { Producer(channel3, 10, 9); }));

        for (std::size_t i = 0; i < producers.size(); i++)
        {
            if (producers[i].joinable())
            {
                producers[i].join();
            }
            else
            {
                std::cerr << "producer  " << i << " Not joinable" << std::endl;
            }
        }

        std::cerr << "ALl producers Finished,  Main thread About to Close Select....\n";
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        selector.Close();
        std::cerr << "Main Closed it\n";

        c1.join();
        c2.join();
        c3.join();
    }

private:
    ChannelSelector selector;
    std::shared_ptr<IChannel<std::string>> channel1;
    std::shared_ptr<IChannel<std::string>> channel2;
    std::shared_ptr<IChannel<std::string>> channel3;
};

class ChannelSourcesOfDataTypesScenario
{
public:
    std::ofstream logFile{"ChannelSourcesOfDataTypesScenario.log.txt"};

    void ProducerString(std::shared_ptr<IChannel<std::string>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue("[std::string] Producer::MEssage from producer thread");
        }
    }

    void ProducerInt(std::shared_ptr<IChannel<int>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue(std::move(i));
        }
    }
    void ProducerDouble(std::shared_ptr<IChannel<double>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue(i + 0.6);
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel1 = std::make_shared<BufferedChannel<std::string>>(100);
            channel2 = std::make_shared<BufferedChannel<int>>(100);
            channel3 = std::make_shared<BufferedChannel<double>>(100);
        }
        else
        {
            channel1 = std::make_shared<UnBufferedChannel<std::string>>();
            channel2 = std::make_shared<UnBufferedChannel<int>>();
            channel3 = std::make_shared<UnBufferedChannel<double>>();
        }

        std::mutex mx;

        selector.AddChannel<std::string>(channel1, [this, &mx](std::string &channelInput)
                                         { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From String-Channel 1: " << channelInput << std::endl; });
        selector.AddChannel<int>(channel2, [this, &mx](int &channelInput)
                                 { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From INT-Channel 2: " << channelInput << std::endl; });
        selector.AddChannel<double>(channel3, [this, &mx](double &channelInput)
                                    { std::lock_guard<std::mutex>lock{mx}; logFile << "Consumer Read value From Double-Channel 3: " << channelInput << std::endl; });

        std::thread c1{&ChannelSourcesOfDataTypesScenario::Consumer, this};
        std::thread c2{&ChannelSourcesOfDataTypesScenario::Consumer, this};
        std::thread c3{&ChannelSourcesOfDataTypesScenario::Consumer, this};

        std::vector<std::thread> producers;
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 30); }));
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 10); }));

        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 20); }));
        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 30); }));
        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 10); }));

        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));

        for (std::size_t i = 0; i < producers.size(); i++)
        {
            if (producers[i].joinable())
            {
                producers[i].join();
            }
            else
            {
                std::cerr << "producer  " << i << " Not joinable" << std::endl;
            }
        }
        std::cerr << "ALl producers Finished,  Main thread About to Close Select....\n";
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        selector.Close();
        std::cerr << "Main Closed it\n";

        c1.join();
        c2.join();
        c3.join();
    }

private:
    ChannelSelector selector;
    std::shared_ptr<IChannel<std::string>> channel1;
    std::shared_ptr<IChannel<int>> channel2;
    std::shared_ptr<IChannel<double>> channel3;
};

class ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario
{
public:
    std::mutex mx;

    std::ofstream logFile{"ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario.log.txt"};

    void ProducerString(std::shared_ptr<IChannel<std::string>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue("[std::string] Producer::MEssage from producer thread");
        }
    }

    void ProducerInt(std::shared_ptr<IChannel<int>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue(std::move(i));
        }
    }
    void ProducerDouble(std::shared_ptr<IChannel<double>> p_pChannel, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            p_pChannel->SendValue(i + 0.6);
        }
    }

    void Consumer()
    {
        while (true)
        {
            bool bResult = selector.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer Exiiting too...\n";
                return;
            }
            // std::lock_guard<std::mutex> lock{mx};
            // logFile << "Result from Selector1 " << std::endl;
        }
    }

    void Consumer2()
    {
        while (true)
        {
            bool bResult = selector2.SelectAndExecute();
            if (!bResult)
            {
                std::cerr << "Consumer2 Exiiting too...\n";
                return;
            }
            // std::lock_guard<std::mutex> lock{mx};
            // logFile << "Result from Selector2 " << std::endl;
        }
    }

    template <typename T>
    void ConsumerFromChannel(std::shared_ptr<IChannel<T>> p_pChannel)
    {

        while (true)
        {
            T tValue;
            bool bResult = p_pChannel->ReadValue(tValue);
            if (!bResult)
            {
                std::cerr << "RawChannel Consumer Exiiting too...\n";
                return;
            }
            std::lock_guard<std::mutex> lock{mx};
            logFile << "Result was read from raw channel" << std::endl;
        }
    }

    void Run()
    {
        if (BUFFERED)
        {
            channel1 = std::make_shared<BufferedChannel<std::string>>(100);
            channel2 = std::make_shared<BufferedChannel<int>>(100);
            channel3 = std::make_shared<BufferedChannel<double>>(100);
        }
        else
        {
            channel1 = std::make_shared<UnBufferedChannel<std::string>>();
            channel2 = std::make_shared<UnBufferedChannel<int>>();
            channel3 = std::make_shared<UnBufferedChannel<double>>();
        }

        selector.AddChannel<std::string>(channel1, [this](std::string &channelInput)
                                         { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector1]:Consumer Read value From String-Channel 1: " << channelInput<<std::endl; });
        selector.AddChannel<int>(channel2, [this](int &channelInput)
                                 { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector1]:Consumer Read value From int-Channel 2: " << channelInput<<std::endl; });
        selector.AddChannel<double>(channel3, [this](double &channelInput)
                                    { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector1]:Consumer Read value From double-Channel 3: " << channelInput<<std::endl; });

        selector2.AddChannel<std::string>(channel1, [this](std::string &channelInput)
                                          { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector2]:Consumer Read value From String-Channel 1: " << channelInput<<std::endl; });
        selector2.AddChannel<int>(channel2, [this](int &channelInput)
                                  { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector2]:Consumer Read value From int-Channel 2: " << channelInput<<std::endl; });
        selector2.AddChannel<double>(channel3, [this](double &channelInput)
                                     { std::lock_guard<std::mutex>lock{mx}; logFile << "[Selector2]:Consumer Read value From double-Channel 3: " << channelInput<<std::endl; });

        std::thread c1{&ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario::Consumer, this};
        std::thread c2{&ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario::Consumer2, this};
        std::thread c3{&ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario::ConsumerFromChannel<std::string>, this, channel1};
        std::thread c4{&ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario::ConsumerFromChannel<int>, this, channel2};
        std::thread c5{&ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario::ConsumerFromChannel<double>, this, channel3};

        std::vector<std::thread> producers;
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 30); }));
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 10); }));

        producers.push_back(std::thread([this]()
                                        { ProducerString(channel1, 10); }));

        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 20); }));
        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 30); }));
        producers.push_back(std::thread([this]()
                                        { ProducerInt(channel2, 10); }));

        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));
        producers.push_back(std::thread([this]()
                                        { ProducerDouble(channel3, 10); }));

        for (std::size_t i = 0; i < producers.size(); i++)
        {
            if (producers[i].joinable())
            {
                producers[i].join();
            }
            else
            {
                std::cerr << "producer  " << i << " Not joinable" << std::endl;
            }
        }
        std::cerr << "ALl producers Finished,  Main thread About to Close Select....\n";
        if (BUFFERED)
        {
            std::cerr << "Sleeping casue this is a buffer (async) channel , consumer may not be finished" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }

        selector.Close();
        selector2.Close();
        channel1->Close();
        channel2->Close();
        channel3->Close();

        std::cerr << "Main Closed it\n";

        c1.join();
        c2.join();
        c3.join();
        c4.join();
        c5.join();
    }

private:
    ChannelSelector selector;
    ChannelSelector selector2;
    std::shared_ptr<IChannel<std::string>> channel1;
    std::shared_ptr<IChannel<int>> channel2;
    std::shared_ptr<IChannel<double>> channel3;
};

int main()
{
    ConsumersAccessingChannelsDirectlyAndConsumersWithDiffSelectsScenario test;
    test.Run();
    std::cerr << "Process exit..\n";
    return 0;
}