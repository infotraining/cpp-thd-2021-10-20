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
#include <condition_variable>

using namespace std::literals;

namespace BusyWait
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

namespace IdleWait
{
    class Data
    {
        std::vector<int> data_;
        bool is_data_ready_{false};
        std::mutex mtx_data_ready_;
        std::condition_variable cv_data_ready_;
    public:
        void read()
        {
            std::cout << "Start reading..." << std::endl;

            data_.resize(100);
            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&] { return rnd() % 1000;} );
            std::this_thread::sleep_for(2s);

            std::cout << "End reading..." << std::endl;

            {
                std::lock_guard<std::mutex> lk{mtx_data_ready_};
                is_data_ready_ = true;
            }

            cv_data_ready_.notify_all();
        }

        void process(int id)
        {
            std::unique_lock<std::mutex> lk{mtx_data_ready_};

            //        while(!is_data_ready_)
            //        {
            //            cv_data_ready_.wait(lk);
            //        }

            cv_data_ready_.wait(lk, [this] { return is_data_ready_; });

            lk.unlock();

            long sum = std::accumulate(begin(data_), end(data_), 0L);

            std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    IdleWait::Data data;

    std::thread thd_producer{[&data] { data.read(); } };

    std::thread thd_consumer1{[&data] { data.process(1); }};
    std::thread thd_consumer2{[&data] { data.process(2); }};

    thd_producer.join();
    thd_consumer1.join();
    thd_consumer2.join();

    std::cout << "Main thread ends..." << std::endl;
}
