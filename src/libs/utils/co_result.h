// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "result.h"

#include <coroutine>
#include <optional>

// return_object_holder adapted from https://github.com/toby-allsopp/coroutine_monad/blob/master/return_object_holder.h
template<typename T>
using deferred = std::optional<T>;

template<typename T>
struct return_object_holder
{
    // The staging object that is returned (by copy/move) to the caller of the coroutine.
    deferred<T> stage;
    return_object_holder *&p;

    // When constructed, we assign a pointer to ourselves to the supplied reference to
    // pointer.
    return_object_holder(return_object_holder *&p)
        : stage{}
        , p(p)
    {
        p = this;
    }

    // Copying doesn't make any sense (which copy should the pointer refer to?).
    return_object_holder(return_object_holder const &) = delete;
    // To move, we just update the pointer to point at the new object.
    return_object_holder(return_object_holder &&other)
        : stage(std::move(other.stage))
        , p(other.p)
    {
        p = this;
    }

    // Assignment doesn't make sense.
    void operator=(return_object_holder const &) = delete;
    void operator=(return_object_holder &&) = delete;

    // Construct the staging value; arguments are perfect forwarded to T's constructor.
    template<typename... Args>
    void emplace(Args &&...args)
    {
        // std::cout << this << ": emplace" << std::endl;
        stage.emplace(std::forward<Args>(args)...);
    }

    // We assume that we will be converted only once, so we can move from the staging
    // object. We also assume that `emplace` has been called at least once.
    operator T()
    {
        // std::cout << this << ": operator T" << std::endl;
        return std::move(*stage);
    }
};

template<typename T>
auto make_return_object_holder(return_object_holder<T> *&p)
{
    return return_object_holder<T>{p};
}

using std::coroutine_traits;

template<typename R>
using p_t = coroutine_traits<Utils::Result<R>>::promise_type;

template<typename E>
struct expectation_failed
{
    E value;
};

template<typename R, typename... Args>
struct coroutine_traits<Utils::Result<R>, Args...>
{
    struct promise_type
    {
        return_object_holder<Utils::Result<R>> *value;
        auto get_return_object() { return make_return_object_holder(value); }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        template<typename R_ = R>
        requires(!std::is_void_v<R_>) void return_value(const R_ &ret)
        {
            value->emplace(std::move(ret));
        }
        void return_value(const Utils::ResultError &err) { value->emplace(std::move(err)); }

        void unhandled_exception() { std::rethrow_exception(std::current_exception()); }
    };
};

template<typename R>
struct result_awaiter
{
    Utils::Result<R> value;
    bool await_ready() { return value.has_value(); }
    void await_suspend(auto &&h) { h.promise().value->emplace(Utils::ResultError{value.error()}); }

    template<typename R_ = R>
    requires(!std::is_void_v<R>) R await_resume()
    {
        return value.value();
    }

    template<typename R_ = R>
    requires(std::is_void_v<R>) void await_resume()
    {}
};

template<typename R>
result_awaiter<R> operator co_await(const Utils::Result<R> &expected)
{
    return result_awaiter<R>{expected};
}
