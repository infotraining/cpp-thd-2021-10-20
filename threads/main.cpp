#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "joining_thread.hpp"

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

class Logger
{
    const int id_;
public:
    explicit Logger(int id) : id_{id}
    {}

    void log(const std::string& message) const
    {
        std::cout << "Log#" << id_ << " - " << message << std::endl;
    }
};


class Lambda_4723478236478
{
public:
    void operator()() const { std::cout << "lambdan"; };
};

class Lambda_8236582375687
{
    const int x_;
    int& y_;
public:
    Lambda_8236582375687(int x, int& y) : x_{x}, y_{y}
    {}

    void operator()() const
    {
        { std::cout << x_ + y_ << "\n"; }
    }
};

void lambda_explain()
{
    auto l = []{ std::cout << "lambda"; };
    auto l_explain = Lambda_4723478236478();

    l();
    l_explain();

    int x = 10;
    int y = 20;

    auto capture = [x, &y] { std::cout << x + y << "\n"; };
    auto capture_explain = Lambda_8236582375687(x, y);

    x = 100;
    y = 200;

    capture();
    capture_explain();
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd_empty;
    std::cout << "thd_empty id: " << thd_empty.get_id() << std::endl;

    {
        ext::joining_thread thd1{&background_work, 1, std::cref(text), 250ms}; // 1 - passing a function address
        ext::joining_thread thd2{&background_work, 2, "threads", 500ms};
    }

    BackgroundWork bw{3, "BackgroundWorker"};
    std::thread thd3{std::ref(bw), 100ms}; // 2 - passing an object function
    std::thread thd4{BackgroundWork{4, "Four"}, 200ms};

    std::thread thd5{[] { background_work(5, "Five", 500ms); }}; // 3 - passing lambda expression

    {
        Logger logger{665};

        std::thread thd_log{[&logger] { logger.log("Logging in another thread"); }};
        thd_log.detach();
    } // possible core-dump - thread is working longer than lifetime of logger

    thd3.join();
    thd4.join();
    thd5.join();


    std::cout << "Main thread ends..." << std::endl;

    lambda_explain();
}
