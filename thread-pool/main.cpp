#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <future>
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

    template <typename Callable>
    auto submit(Callable&& callable)
    {
        using ResultT = decltype(callable());

        auto pt = std::make_shared<std::packaged_task<ResultT()>>(std::forward<Callable>(callable));
        std::future<ResultT> f = pt->get_future();
        tasks_.push([pt] { (*pt)(); });

        return f;
    }

    ~ThreadPool()
    {
        // weakup
        for(size_t i = 0; i < threads_.size(); ++i)
            submit([this] { is_done_ = true; });

        for(auto& thd : threads_)
            thd.join();
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

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
}

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}


int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ThreadPool thread_pool(8);

    //thread_pool.submit([&] { background_work(1, text, 100ms); });

    std::vector<std::future<int>> squares;

    for(int i = 1; i <= 20; ++i)
    {
        std::future<int> f_sqr = thread_pool.submit([i] { return calculate_square(i); });
        squares.push_back(std::move(f_sqr));
    }

    for(auto& s : squares)
    {
        try
        {
            int result = s.get();
            std::cout << result << std::endl;
        }
        catch(const std::runtime_error& e)
        {
            std::cout << "Caught: " << e.what() << std::endl;
        }
    }

    std::cout << "Main thread ends..." << std::endl;


    ///////////////////////
    /// ref vs. shared_ptr

    auto sum = [](std::shared_ptr<const std::vector<int>> data)
    {
        long result =  std::accumulate(data->begin(), data->end(), 0);
        std::cout << "Sum: " << result << std::endl;
    };

    {
        auto il = {1, 2, 3, 4, 5};
        std::shared_ptr<const std::vector<int>> data = std::make_shared<const std::vector<int>>(il);

        std::thread thd1{sum, data};
        thd1.detach();

        std::thread thd2{[sum, data] { std::this_thread::sleep_for(10s);
            sum(data);}
        };

        thd2.detach();
    }

    std::this_thread::sleep_for(15s);
}
