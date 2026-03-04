// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstddef>
#include <functional>

namespace Utils {

// Primary: callable objects (lambdas, functors) — deduce via operator()
template<typename F>
struct first_arg_of_callable_impl : first_arg_of_callable_impl<decltype(&F::operator())>
{};

// Base case: plain function type
template<typename R, typename T, typename... Args>
struct first_arg_of_callable_impl<R(T, Args...)>
{
    using type = T;
};

// Function pointer
template<typename R, typename... Args>
struct first_arg_of_callable_impl<R (*)(Args...)> : first_arg_of_callable_impl<R(Args...)>
{};

// Member function pointer (non-const and const)
template<typename C, typename R, typename... Args>
struct first_arg_of_callable_impl<R (C::*)(Args...)> : first_arg_of_callable_impl<R(Args...)>
{};

template<typename C, typename R, typename... Args>
struct first_arg_of_callable_impl<R (C::*)(Args...) const> : first_arg_of_callable_impl<R(Args...)>
{};

template<typename F>
using first_arg_of_callable = typename first_arg_of_callable_impl<F>::type;

template<typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// F is directly invocable with T
template<typename F, typename T>
concept FunctionTakingBaseType = std::is_invocable_v<F, T>;

// TF is a derived pointer of T (both raw pointers)
template<typename F, typename T, typename TF = first_arg_of_callable<F>>
concept FunctionTakingDerivedPointer
    = !std::is_same_v<TF, T>
      && std::is_base_of_v<std::remove_pointer_t<T>, std::remove_pointer_t<TF>>;

// TF is a shared_ptr<Y> convertible to shared_ptr<X> where Y derives from X
template<typename F, typename T, typename TF = first_arg_of_callable<F>>
concept FunctionTakingDerivedSharedPtr
    = !std::is_same_v<TF, T> && std::is_convertible_v<TF, T>
      && !std::is_same_v<typename base_type<T>::element_type, typename base_type<TF>::element_type>;

// TF is a raw pointer to a type derived from the element_type of shared_ptr T
template<typename F, typename T, typename TF = first_arg_of_callable<F>>
concept FunctionTakingRawPointer
    = !std::is_same_v<TF, T>
      && std::is_base_of_v<typename base_type<T>::element_type, std::remove_pointer_t<TF>>;

template<typename R, typename T>
class SingleArgCallback
{
    using BaseFunc = std::function<R(const T &)>;
    BaseFunc baseFunc;

public:
    R operator()(const T &t) { return baseFunc(t); }

    SingleArgCallback() = default;
    SingleArgCallback(const SingleArgCallback &other) = delete;

    // T = X, func(Y), Y derives from X
    SingleArgCallback &operator=(const FunctionTakingDerivedPointer<T> auto &func)
    {
        baseFunc = [f = func](const T &t) mutable -> R {
            using TF = first_arg_of_callable<base_type<decltype(func)>>;
            return f(static_cast<TF>(t));
        };
        return *this;
    }

    // T = std::shared_ptr<X>, func(std::shared_ptr<Y>), Y is derived from X
    SingleArgCallback &operator=(const FunctionTakingDerivedSharedPtr<T> auto &func)
    {
        baseFunc = [f = func](const T &t) mutable -> R {
            using TF = first_arg_of_callable<base_type<decltype(func)>>;
            return f(std::static_pointer_cast<typename base_type<TF>::element_type>(t));
        };
        return *this;
    }

    // T = std::shared_ptr<X>, func(Y), Y is derived from X
    SingleArgCallback &operator=(const FunctionTakingRawPointer<T> auto &func)
    {
        baseFunc = [f = func](const T &t) mutable -> R {
            using TF = first_arg_of_callable<base_type<decltype(func)>>;
            return f(std::static_pointer_cast<std::remove_pointer_t<TF>>(t).get());
        };
        return *this;
    }

    // func is directly invocable with T
    SingleArgCallback &operator=(const FunctionTakingBaseType<T> auto &func)
    {
        baseFunc = func;
        return *this;
    }

    SingleArgCallback &operator=(const SingleArgCallback &other) = delete;

    operator bool() const { return (bool) baseFunc; }
};

} // namespace Utils
