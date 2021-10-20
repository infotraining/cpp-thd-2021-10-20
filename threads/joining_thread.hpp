#ifndef JOINING_THREAD_CPP
#define JOINING_THREAD_CPP

#include <thread>
#include <type_traits>

namespace ext
{
    template <typename T1, typename T2>
    constexpr bool is_similar_v = std::is_same<std::decay_t<T1>, std::decay_t<T2>>::value;

    class joining_thread
    {
        std::thread thd_;

    public:
        joining_thread() = default;

        template <typename Callable, typename... Args,
            typename = std::enable_if_t<!is_similar_v<Callable, joining_thread>>>
        joining_thread(Callable&& callable, Args&&... args)
            : thd_ {std::forward<Callable>(callable), std::forward<Args>(args)...}
        {
        }

        joining_thread(const joining_thread&) = delete;
        joining_thread& operator=(const joining_thread&) = delete;

        joining_thread(joining_thread&&) = default;
        joining_thread& operator=(joining_thread&&) = default;

        std::thread& get()
        {
            return thd_;
        }

        void join()
        {
            thd_.join();
        }

        void detach()
        {
            thd_.detach();
        }

        bool joinable() const noexcept
        {
            return thd_.joinable();
        }

        std::thread::id get_id() const noexcept
        {
            return thd_.get_id();
        }

        std::thread::native_handle_type native_handle()
        {
            return thd_.native_handle();
        }

        ~joining_thread()
        {
            if (thd_.joinable())
                thd_.join();
        }
    };
}

#endif
