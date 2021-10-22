#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <random>
#include <future>

using namespace std;

void bg_pi(size_t N, long &hits)
{
    std::random_device rd;
    std::mt19937_64 gen{rd()};
    std::uniform_real_distribution<double> rnd_gen(-1.0, 1.0);

    long local_hits = 0;

    for (long n = 0; n < N; ++n)
    {
        double x = rnd_gen(gen);
        double y = rnd_gen(gen);

        if (x * x + y * y < 1)
            local_hits++;
    }

    hits += local_hits;
}

long calc_hits(size_t N)
{
    std::random_device rd;
    std::mt19937_64 gen{rd()};
    std::uniform_real_distribution<double> rnd_gen(-1.0, 1.0);

    long local_hits = 0;

    for (long n = 0; n < N; ++n)
    {
        double x = rnd_gen(gen);
        double y = rnd_gen(gen);

        if (x * x + y * y < 1)
            local_hits++;
    }

    return local_hits;
}


int main()
{
    const long N = 100'000'000;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //////////////////////////////////////////////////////////////////////////////
    // single thread

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    long hits = 0;

    bg_pi(N, hits);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "SINGLE Pi = " << pi << endl;
    cout << "SINGLE Elapsed = " << elapsed_time << "ms" << endl;

    //Bylo 3752ms

    //////////////////////////////////////////////////////////////////////////////
    ///
    ///

    const unsigned n_of_cores = std::max(1u, std::thread::hardware_concurrency() );

    std::cout << "n_of_cores: " << n_of_cores << "\n";

    const long N_part = N/n_of_cores;

    {
        cout << "Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        std::vector<long> results(n_of_cores, 0);
        std::vector<std::thread> thds(n_of_cores);

        for(int i=0; i<n_of_cores; ++i)
            thds[i] = std::thread{&bg_pi, N_part, std::ref(results[i]) };

        for(auto &thd : thds)
            thd.join();

        long hits = std::accumulate(std::begin(results), std::end(results), 0L);

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "MULTI Pi = " << pi << endl;
        cout << "MULTI Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////
    ///
    ///

    {
        cout << "Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        std::vector<std::future<long>> results;

        for(int i=0; i < n_of_cores; ++i)
        {
            std::future<long> f = std::async(std::launch::async, &calc_hits, N_part);
            results.push_back(std::move(f));
        }

        long hits = 0L;

        for(auto& h : results)
            hits += h.get();

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Future Pi = " << pi << endl;
        cout << "Future PI Elapsed = " << elapsed_time << "ms" << endl;
    }
}
