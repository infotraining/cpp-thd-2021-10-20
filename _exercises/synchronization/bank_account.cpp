#include <iostream>
#include <thread>
#include <mutex>
#include <cassert>

class BankAccount
{
    const int id_;
    double balance_;
    mutable std::recursive_mutex mtx_;

public:
    BankAccount(int id, double balance)
        : id_(id)
        , balance_(balance)
    {
    }

    void print() const
    {
        std::cout << "Bank Account #" << id_ << "; Balance = " << balance() << std::endl;
    }

    void transfer(BankAccount& to, double amount)
    {
        assert(this != &to);

        // 1st way
        {
            std::unique_lock<std::recursive_mutex> lk_from{mtx_, std::defer_lock};
            std::unique_lock<std::recursive_mutex> lk_to{to.mtx_, std::defer_lock};

            std::lock(lk_from, lk_to); // SC begin

            balance_ -= amount;
            to.balance_ += amount;
        } // SC ends

//        //2nd way
//        {
//            std::lock(mtx_, to.mtx_); // SC begins
//            std::unique_lock<std::recursive_mutex> lk_from{mtx_, std::adopt_lock};
//            std::unique_lock<std::recursive_mutex> lk_to{to.mtx_, std::adopt_lock};

//            balance_ -= amount;
//            to.balance_ += amount;
//        } // SC ends


#if __cplusplus > 201703L
        {
            std::scoped_lock lks{mtx_, to.mtx_};

            balance_ -= amount;
            to.balance_ += amount;
        }

#endif


    } // SC end

    void withdraw(double amount)
    {
        std::lock_guard<std::recursive_mutex> lk{mtx_};
        balance_ -= amount;
    }

    void deposit(double amount)
    {
        std::lock_guard<std::recursive_mutex> lk{mtx_};
        balance_ += amount;
    }

    int id() const
    {
        return id_;
    }

    double balance() const
    {
        std::lock_guard<std::recursive_mutex> lk{mtx_};
        return balance_;
    }

    void lock()
    {
        mtx_.lock();
    }

    void unlock()
    {
        mtx_.unlock();
    }

    bool try_lock()
    {
        return mtx_.try_lock();
    }

    [[nodiscard]] std::unique_lock<std::recursive_mutex> start_transaction()
    {
        return std::unique_lock<std::recursive_mutex>{mtx_};
    }
};

void make_withdraws(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.withdraw(1.0);
}

void make_deposits(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.deposit(1.0);
}

void make_transfers(BankAccount& from, BankAccount& to, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        from.transfer(to, 1.0);
}

int main()
{
    const int NO_OF_ITERS = 10'000'000;

    BankAccount ba1(1, 10'000);
    BankAccount ba2(2, 10'000);

    std::cout << "Before threads are started: ";
    ba1.print();
    ba2.print();

    std::thread thd1(&make_withdraws, std::ref(ba1), NO_OF_ITERS);
    std::thread thd2(&make_deposits, std::ref(ba1), NO_OF_ITERS);
    std::thread thd3{&make_transfers, std::ref(ba1), std::ref(ba2), NO_OF_ITERS};
    make_transfers(ba2, ba1, NO_OF_ITERS);

    {
        auto transaction_scope = ba1.start_transaction();
        ba1.deposit(100);
        ba1.deposit(200);
        ba1.withdraw(300);
    }

    thd1.join();
    thd2.join();
    thd3.join();

    std::cout << "After all threads are done: ";
    ba1.print();
    ba2.print();
}
