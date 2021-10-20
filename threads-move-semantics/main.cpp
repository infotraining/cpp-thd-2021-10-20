#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

std::thread create_background_work()
{
    static int id = 100;
    const int current_id = ++id;
    const std::string message = "Thread#" + std::to_string(current_id);

    return std::thread{&background_work, id, message, 100ms};
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd1{&background_work, 1, "hello", 200ms};

    std::thread thd2 = std::move(thd1);

    thd1 = create_background_work();

    std::thread thd3 = create_background_work();

    std::vector<std::thread> threads;
    threads.push_back(std::move(thd1));
    threads.push_back(std::move(thd2));
    threads.push_back(std::move(thd3));
    threads.push_back(create_background_work());

    for(auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    std::cout << "Main thread ends..." << std::endl;
}
