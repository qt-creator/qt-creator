struct promise;
struct coroutine : std::coroutine_handle<promise>
{
    using promise_type = struct promise;
};
struct promise
{
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};
struct S
{
    int i;
    coroutine f()
    {
        std::cout << i;
        co_return;
    }
};
void good()
{
    coroutine h = [](int i) -> coroutine
    {
        std::cout << i;
        co_return;
    }(0);
    h.resume();
    h.destroy();
}
auto switch_to_new_thread(std::jthread& out)
{
    struct awaitable
    {
        std::jthread* p_out;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h)
        {
            std::jthread& out = *p_out;
            if (out.joinable())
                throw std::runtime_error("Output jthread parameter not empty");
            out = std::jthread([h] { h.resume(); });
            std::cout << "New thread ID: " << out.get_id() << '\n'; // this is OK
        }
        void await_resume() {}
    };
    return awaitable{&out};
}
struct task
{
    struct promise_type
    {
        task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};
task resuming_on_new_thread(std::jthread& out)
{
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    co_await switch_to_new_thread(out);
    std::cout << "Coroutine resumed on thread: " << std::this_thread::get_id() << '\n';
}
void run()
{
    std::jthread out;
    resuming_on_new_thread(out);
}
template <typename T>
struct Generator
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    struct promise_type // required
    {
        T value_;
        std::exception_ptr exception_;

        Generator get_return_object()
        {
            return Generator(handle_type::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = std::current_exception(); }
        template <std::convertible_to<T> From>
        std::suspend_always yield_value(From&& from)
        {
            value_ = std::forward<From>(from);
            return {};
        }
        void return_void() { }
    };
    handle_type h_;
    Generator(handle_type h) : h_(h) {}
    ~Generator() { h_.destroy(); }
    explicit operator bool()
    {
        fill();
        return !h_.done();
    }
    T operator()()
    {
        fill();
        full_ = false;
        return std::move(h_.promise().value_);
    }
private:
    bool full_ = false;
    void fill()
    {
        if (!full_)
        {
            h_();
            if (h_.promise().exception_)
                std::rethrow_exception(h_.promise().exception_);
            full_ = true;
        }
    }
};
Generator<std::uint64_t>
fibonacci_sequence(unsigned n)
{
    if (n == 0)
        co_return;
    if (n > 94)
        throw std::runtime_error("Too big Fibonacci sequence. Elements would overflow.");
    co_yield 0;
    if (n == 1)
        co_return;
    co_yield 1;
    if (n == 2)
        co_return;
    std::uint64_t a = 0;
    std::uint64_t b = 1;
    for (unsigned i = 2; i < n; i++)
    {
        std::uint64_t s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}
int main()
{
    try
    {
        auto gen = fibonacci_sequence(10); // max 94 before uint64_t overflows
        for (int j = 0; gen; j++)
            std::cout << "fib(" << j << ")=" << gen() << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
    }
}
