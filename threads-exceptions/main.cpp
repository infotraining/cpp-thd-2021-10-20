#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
struct ThreadResult
{
    T value;
    std::exception_ptr eptr;

    T get() const
    {
        if (eptr)
            std::rethrow_exception(eptr);
        return value;
    }
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay, ThreadResult<char>& result)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << " - std::thread::id: " << std::this_thread::get_id() << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        //throw 13;
        result.value = text.at(2);

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
    catch(...)
    {
        std::cout << "Something has been caught!" << std::endl;
        result.eptr = std::current_exception();
        return;
    }
}

int main()
{
    std::cout << "Number of cores: " << std::max(1u, std::thread::hardware_concurrency()) << "\n";

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    try
    {
        ThreadResult<char> letter;
        std::thread thd1{&background_work, 1, "ok", 100ms, std::ref(letter)};

        thd1.join();

        std::cout << "Third letter: " << letter.get() << "\n";
    }
    catch(const std::out_of_range& e)
    {
        std::cout << "Caught an exception: " << e.what() << std::endl;
    }
    catch(int e)
    {
        std::cout << "Caught an int: " << e << std::endl;
    }

    //////////////////////////////////
    /// grouping threads

    {
        std::array<ThreadResult<char>, 2> results;
        std::vector<std::thread> thds(2);

        thds[0] = std::thread{&background_work, 1, "Text", 200ms, std::ref(results[0])};
        thds[1] = std::thread{&background_work, 2, "OK", 200ms, std::ref(results[1])};

        for(auto& thd : thds)
            thd.join();

        for(auto& r : results)
        {
            try
            {
                char letter = r.get();
                std::cout << "letter: " << letter << std::endl;
            }
            catch(const std::out_of_range& e)
            {
                std::cout << "Caught an exception: " << e.what() << std::endl;
            }
            catch(int e)
            {
                std::cout << "Caught an int: " << e << std::endl;
            }
        }

    }

    std::cout << "Main thread ends..." << std::endl;
}
