#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

std::timed_mutex mtx;

void background_work(size_t id, std::chrono::milliseconds timeout)
{
    std::cout << "THD#" << id << " is waiting for mutex..." << std::endl;

    std::unique_lock<std::timed_mutex> lk{mtx, std::try_to_lock};

    if (!lk.owns_lock())
    {
        do
        {
            std::cout << "THD#" << id << " does not own a lock..."
                      << " Tries to acquire a mutex..." << std::endl;
        } while(!lk.try_lock_for(timeout));
    }

    std::cout << "THD#" << id << " owns a mutex. Starts its job." << std::endl;
    std::this_thread::sleep_for(10s);
    std::cout << "THD#" << id << " is finished..." << std::endl;
}


int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd1{&background_work, 1, 750ms};
    std::thread thd2{&background_work, 2, 500ms};

    thd1.join();
    thd2.join();

    std::cout << "Main thread ends..." << std::endl;
}
