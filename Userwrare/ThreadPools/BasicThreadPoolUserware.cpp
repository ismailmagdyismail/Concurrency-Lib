#include <iostream>
#include <memory>

#include "BasicThreadPool.h"
#include "UnBufferedChannel.h"

int main()
{
    BasicThreadPool oPool;
    std::function<int(void)> fTask = []()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        return 10;
    };
    std::shared_ptr<UnBufferedChannel<int>> channelResult = oPool.SubmitTask<int>(std::move(fTask));

    std::cerr << "Waitiing for result to be executed by a Worker Thread\n";
    int val;
    bool bReadSuccess = channelResult->ReadValue(val);
    if (!bReadSuccess)
    {
        std::cerr << "Failed to Read Result from Channel, it was probably Closed \n";
        return -1;
    }
    std::cerr << "Value Read :: " << val << '\n';
}