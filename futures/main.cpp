#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

void wtf()
{
    auto f1 = std::async(std::launch::async, &save_to_file, "F1");
    auto f2 = std::async(std::launch::async, &save_to_file, "F2");
    auto f3 = std::async(std::launch::async, &save_to_file, "F3");
    auto f4 = std::async(std::launch::async, &save_to_file, "F4");
}

int async_demo()
{
    std::future<int> f1 = std::async(std::launch::async, &calculate_square, 13);
    std::future<int> f2 = std::async(std::launch::async, &calculate_square, 9);
    std::future<void> f3 = std::async(std::launch::async, [] { save_to_file("Some content"); });

    while ( f1.wait_for(100ms) != std::future_status::ready)
    {
        std::cout << "I'm still waiting..." << std::endl;
    }

    int r1 = f1.get();
    std::cout << "r1: " << r1 << std::endl;

    try
    {
        int r2 = f2.get();
        std::cout << "r2: " << r2 << std::endl;
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "Caught: " << e.what() << std::endl;
    }

    f3.wait();

    std::cout << "\n--------------------------\n" << std::endl;

    wtf();
}

template <typename Callable>
auto launch_async(Callable callable)
{
    using ResultT = decltype(callable());

    std::packaged_task<ResultT()> pt{callable};
    std::future<ResultT> f = pt.get_future();

    std::thread thd{std::move(pt)};
    thd.detach();

    return f;
}


void packaged_task_demo()
{
    std::packaged_task<int()> pt1([] { return calculate_square(13); });
    std::future<int> f1 = pt1.get_future();

    std::thread thd1{std::move(pt1)};
    thd1.detach();

    std::cout << "result: " << f1.get() << std::endl;

    auto s1 = launch_async([] { save_to_file("F1");});
    auto s2 = launch_async([] { save_to_file("F2");});
    auto f = launch_async([] { return calculate_square(13); });

    std::cout << "result: " << f.get() << std::endl;

    s2.wait();
}

class SquareCalculator
{
    std::promise<int> promise_;
public:
    void calculate(int x)
    {
        try
        {
            int result = calculate_square(x);
            promise_.set_value(result);
        }
        catch (...)
        {
            promise_.set_exception(std::current_exception());
        }
    }

    std::future<int> get_future()
    {
        return promise_.get_future();
    }
};

void promise_demo()
{
    SquareCalculator calc;

    std::future<int> f = calc.get_future();
    std::shared_future shared_f = f.share();

    std::thread thd_consumer{[shared_f] {
            std::cout << "Consuming: " << shared_f.get() << std::endl;
        }
    };

    std::thread thd_calc{[&calc] { calc.calculate(17); } };

    std::cout << "17 * 17 = " << shared_f.get() << std::endl;

    thd_calc.join();
    thd_consumer.join();
}

int main()
{
    //packaged_task_demo();

    promise_demo();
}
