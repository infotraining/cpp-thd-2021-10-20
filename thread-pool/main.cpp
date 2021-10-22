#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include "thread_safe_queue.hpp"

using namespace std::literals;

using Task = std::function<void()>;

const Task end_of_work;

namespace ver_1_0
{
    class ThreadPool
    {
        std::vector<std::thread> threads_;
        ThreadSafeQueue<Task> tasks_;

        void run()
        {
            while(true)
            {
                Task task;
                tasks_.pop(task);

                if (is_poisonning_pill(task))
                    return;

                task(); // execution of task
            }
        }

        bool is_poisonning_pill(const Task& t)
        {
            return t == nullptr;
        }
    public:
        ThreadPool(size_t size) : threads_(size)
        {
            for(size_t i = 0; i < size; ++i)
                threads_[i] = std::thread{[this] { run(); }};
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            // sending poisonnig pills
            for(size_t i = 0; i < threads_.size(); ++i)
                tasks_.push(end_of_work);

            for(auto& thd : threads_)
                thd.join();
        }

        void submit(Task task)
        {
            assert(task != nullptr);

            tasks_.push(task);
        }
    };
}

class ThreadPool
{
    std::vector<std::thread> threads_;
    ThreadSafeQueue<Task> tasks_;
    std::atomic<bool> is_done_{false};

    void run()
    {
        while(true)
        {
            Task task;
            tasks_.pop(task);

            task(); // execution of task

            if (is_done_)
                return;
        }
    }

public:
    ThreadPool(size_t size) : threads_(size)
    {
        for(size_t i = 0; i < size; ++i)
            threads_[i] = std::thread{[this] { run(); }};
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        // weakup
        for(size_t i = 0; i < threads_.size(); ++i)
            submit([this] { is_done_ = true; });

        for(auto& thd : threads_)
            thd.join();
    }

    void submit(Task task)
    {
        tasks_.push(task);
    }
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ThreadPool thread_pool(4);

    thread_pool.submit([&] { background_work(1, text, 100ms); });

    for(int i = 0; i < 100; ++i)
    {
        thread_pool.submit([i, &text] { background_work(i, text, 20ms);} );
    }

    std::cout << "Main thread ends..." << std::endl;


}
