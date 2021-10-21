#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

class Counter
{
    int counter = 0;
    mutable std::mutex mtx_counter;
public:
    void increment()
    {
        std::lock_guard<std::mutex> lk{mtx_counter};  // begin of critical section
        counter++;
    } // end of critical section

    int value() const
    {
        std::lock_guard<std::mutex> lk{mtx_counter};
        return counter;
    }
};

void increment(Counter& counter)
{
    for(int i = 0; i < 1'000'000; ++i)
    {
        counter.increment();
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    Counter counter;

    std::thread thd1{&increment, std::ref(counter)};
    std::thread thd2{[&] { increment(counter); }};

    thd1.join();
    thd2.join();

    std::cout << "counter: " << counter.value() << std::endl;

    std::cout << "Main thread ends..." << std::endl;
}
