#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <algorithm>
#include <atomic>

using namespace std::literals;

namespace atomics
{

class Data
{
    std::vector<int> data_;
    std::atomic<bool> is_data_ready_{false};
public:
    void read()
    {
        std::cout << "Start reading..." << std::endl;

        data_.resize(100);
        std::random_device rnd;
        std::generate(begin(data_), end(data_), [&] { return rnd() % 1000;} );
        std::this_thread::sleep_for(2s);

        std::cout << "End reading..." << std::endl;

        is_data_ready_.store(true);
    }

    void process(int id)
    {
        while (!is_data_ready_.load()) // busy wait
        {}

        long sum = std::accumulate(begin(data_), end(data_), 0L);

        std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

}

class Data
{
    std::vector<int> data_;
    bool is_data_ready_{false};
public:
    void read()
    {
        std::cout << "Start reading..." << std::endl;

        data_.resize(100);
        std::random_device rnd;
        std::generate(begin(data_), end(data_), [&] { return rnd() % 1000;} );
        std::this_thread::sleep_for(2s);

        std::cout << "End reading..." << std::endl;

        is_data_ready_ = true;
    }

    void process(int id)
    {
        while (!is_data_ready_)
        {}

        long sum = std::accumulate(begin(data_), end(data_), 0L);

        std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    atomics::Data data;

    std::thread thd_producer{[&data] { data.read(); } };

    std::thread thd_consumer1{[&data] { data.process(1); }};
    std::thread thd_consumer2{[&data] { data.process(2); }};

    thd_producer.join();
    thd_consumer1.join();
    thd_consumer2.join();

    std::cout << "Main thread ends..." << std::endl;
}
