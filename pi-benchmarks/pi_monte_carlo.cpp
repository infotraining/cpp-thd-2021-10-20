#define CATCH_CONFIG_ENABLE_BENCHMARKING
#define CATCH_CONFIG_MAIN

#include <catch.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <future>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <new>

using namespace std;

template <typename Real>
class RandomEngine
{    
    mt19937 gen_;
    uniform_real_distribution<Real> dis_;

public:
    RandomEngine()
        : gen_([] { 
            random_device rd;
            seed_seq seed{rd(), rd(), rd(), rd(), rd(), rd()};
            return mt19937(seed);
            }())
        , dis_(-1, 1)
    {
    }

    Real operator()()
    {
        return dis_(gen_);
    }
};

using counter_t = unsigned long long int;

counter_t calc_hits(counter_t n)
{
    RandomEngine<double> rnd;

    counter_t counter = 0;

    for (counter_t i = 0; i < n; ++i)
    {
        double x = rnd();
        double y = rnd();
        if (x * x + y * y < 1)
            counter++;
    }

    return counter;
}

void calc_hits_by_ref_intensive_incr(counter_t n, counter_t& hits)
{
    RandomEngine<double> rnd;

    for (counter_t i = 0; i < n; ++i)
    {
        double x = rnd();
        double y = rnd();
        if (x * x + y * y < 1)
            hits++;
    }
}

void calc_hits_by_local_counter(counter_t n, counter_t& hits)
{
    RandomEngine<double> rnd;

    counter_t counter = 0;

    for (counter_t i = 0; i < n; ++i)
    {
        double x = rnd();
        double y = rnd();
        if (x * x + y * y < 1)
            counter++;
    }

    hits += counter;
}

void calc_hits_by_ref_intensive_incr_with_mutex(counter_t n, counter_t& hits, mutex& mtx)
{
    RandomEngine<double> rnd;

    counter_t local_hits{};
    for (counter_t i = 0; i < n; ++i)
    {
        double x = rnd();
        double y = rnd();
        if (x * x + y * y < 1)
        {            
            local_hits++;
        }
    }

    {
        std::lock_guard<std::mutex> lk{mtx};
        hits += local_hits;
    }
}

void calc_hits_by_ref_intensive_incr_with_atomic(counter_t n, atomic<counter_t>& hits)
{
    RandomEngine<double> rnd;

    for (counter_t i = 0; i < n; ++i)
    {
        double x = rnd();
        double y = rnd();
        if (x * x + y * y < 1)
        {
            hits.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

double calc_pi_single_thread(counter_t throws)
{
    return static_cast<double>(calc_hits(throws)) / throws * 4;
}

double calc_pi_single_thread_with_mutex(counter_t throws)
{
    counter_t hits = 0;
    mutex mtx;

    calc_hits_by_ref_intensive_incr_with_mutex(throws, hits, mtx);

    return static_cast<double>(hits) / throws * 4;
}

double calc_pi_multithread1(counter_t throws)
{
    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto throws_per_thread = throws / hardware_threads_count;

    vector<thread> threads;
    vector<counter_t> hits(hardware_threads_count);

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, i, throws_per_thread] { hits[i] = calc_hits(throws_per_thread); });
    }

    for (auto& thd : threads)
        thd.join();

    return (accumulate(hits.begin(), hits.end(), 0.0) / throws) * 4;
}

double calc_pi_multithread_with_mutex(counter_t throws)
{
    counter_t hits = 0;
    mutex mtx;

    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto no_of_throws = throws / hardware_threads_count;

    vector<thread> threads;

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, &mtx, no_of_throws] {
            calc_hits_by_ref_intensive_incr_with_mutex(no_of_throws, hits, mtx);
        });
    }

    for (auto& thd : threads)
        thd.join();

    return (static_cast<double>(hits) / throws) * 4;
}

double calc_pi_multithread_with_atomic(counter_t throws)
{
    atomic<counter_t> hits{};

    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto no_of_throws = throws / hardware_threads_count;

    vector<thread> threads;

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, no_of_throws] {
            calc_hits_by_ref_intensive_incr_with_atomic(no_of_throws, hits);
        });
    }

    for (auto& thd : threads)
        thd.join();

    return (static_cast<double>(hits) / throws) * 4;
}

double calc_pi_multithread_false_sharing(counter_t throws)
{
    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto no_of_throws = throws / hardware_threads_count;

    vector<thread> threads;
    vector<counter_t> hits(hardware_threads_count);

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, i, no_of_throws] { calc_hits_by_ref_intensive_incr(no_of_throws, hits[i]); });
    }

    for (auto& thd : threads)
        thd.join();

    return (accumulate(hits.begin(), hits.end(), 0.0) / throws) * 4;
}

double calc_pi_multithread_with_padding(counter_t throws)
{
    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto no_of_throws = throws / hardware_threads_count;

    struct alignas(128) AlignedValue
    {
        counter_t value;
    };

    vector<thread> threads;
    vector<AlignedValue> hits(hardware_threads_count);

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, i, no_of_throws] { calc_hits_by_ref_intensive_incr(no_of_throws, hits[i].value); });
    }

    for (auto& thd : threads)
        thd.join();

    return (accumulate(hits.begin(), hits.end(), 0.0, [](double a, const AlignedValue& av) { return a + av.value; }) / throws) * 4;
}

double calc_pi_multithread_local_counter(counter_t throws)
{
    auto hardware_threads_count = max(thread::hardware_concurrency(), 1u);
    auto no_of_throws = throws / hardware_threads_count;

    vector<thread> threads;
    vector<counter_t> hits(hardware_threads_count);

    for (unsigned int i = 0; i < hardware_threads_count; ++i)
    {
        threads.emplace_back([&hits, i, no_of_throws] { calc_hits_by_local_counter(no_of_throws, hits[i]); });
    }

    for (auto& thd : threads)
        thd.join();

    return (accumulate(hits.begin(), hits.end(), 0.0) / throws) * 4;
}

constexpr int N = 1'000'000;

TEST_CASE("Monte Carlo Pi")
{
    BENCHMARK("single threaded") {
        return calc_pi_single_thread(N);
    };

    BENCHMARK("synchronized using mutex") {
        return calc_pi_multithread_with_mutex(N);
    };

    BENCHMARK("single thread with mutex") {
        return calc_pi_single_thread_with_mutex(N);
    };

    BENCHMARK("with atomics") {
        return calc_pi_multithread_with_atomic(N);
    };

    BENCHMARK("local counter") {
        return calc_pi_multithread_local_counter(N);
    };

    BENCHMARK("false sharing") {
        return calc_pi_multithread_false_sharing(N);
    };

    BENCHMARK("padding") {
        return calc_pi_multithread_with_padding(N);
    };
}
