// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>

namespace Utils {

// Primary: SFINAE-friendly fallback — no type for non-callable F
template<typename F, typename = void>
struct first_arg_of_callable_impl
{};

// Callable objects (lambdas, functors) — deduce via operator()
template<typename F>
struct first_arg_of_callable_impl<F, std::void_t<decltype(&F::operator())>>
    : first_arg_of_callable_impl<decltype(&F::operator())>
{};

// Base case: plain function type
template<typename R, typename T, typename... Args>
struct first_arg_of_callable_impl<R(T, Args...), void>
{
    using type = T;
};

// Function pointer
template<typename R, typename... Args>
struct first_arg_of_callable_impl<R (*)(Args...), void> : first_arg_of_callable_impl<R(Args...)>
{};

// Member function pointer (non-const and const)
template<typename C, typename R, typename... Args>
struct first_arg_of_callable_impl<R (C::*)(Args...), void> : first_arg_of_callable_impl<R(Args...)>
{};

template<typename C, typename R, typename... Args>
struct first_arg_of_callable_impl<R (C::*)(Args...) const, void>
    : first_arg_of_callable_impl<R(Args...)>
{};

template<typename F>
using first_arg_of_callable = typename first_arg_of_callable_impl<F>::type;

template<typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// Check if F has a deducible first argument
template<typename F>
concept HasCallableFirstArg = requires
{
    typename first_arg_of_callable_impl<F>::type;
};

// MSVC workaround: always-valid version of first_arg_of_callable that returns Default
// when F has no deducible first argument (e.g. zero-arg member function pointers).
// Needed because older MSVC evaluates alias templates in discarded if-constexpr branches.
template<typename F, typename Default = void>
using first_arg_or = typename std::conditional_t<
    HasCallableFirstArg<F>,
    first_arg_of_callable_impl<F>,
    std::type_identity<Default>>::type;

// F's first arg differs from T (and F is callable)
template<typename F, typename T>
concept DifferentFirstArg = HasCallableFirstArg<F> && !std::is_same_v<first_arg_of_callable<F>, T>;

// F is directly invocable with T (and optional extra args)
template<typename F, typename T, typename... ExtraArgs>
concept DirectlyInvocable = std::is_invocable_v<F, T, ExtraArgs...>;

// Extract class type from a member function pointer
template<typename>
struct member_class_of;
template<typename C, typename R, typename... MArgs>
struct member_class_of<R (C::*)(MArgs...)>
{
    using type = C;
};
template<typename C, typename R, typename... MArgs>
struct member_class_of<R (C::*)(MArgs...) const>
{
    using type = C;
};

// F's first arg is covariant with T: derived pointer, derived shared_ptr, raw ptr from shared_ptr,
// or member function pointer on a derived class
template<typename F, typename T, typename... ExtraArgs>
concept TakesCovariantArg =
    // callable with covariant first argument
    (DifferentFirstArg<F, T>
     && std::is_invocable_v<F, first_arg_of_callable<F>, ExtraArgs...>
     && (
         (std::is_pointer_v<T> && std::is_pointer_v<first_arg_of_callable<F>>
          && std::is_base_of_v<std::remove_pointer_t<T>,
                               std::remove_pointer_t<first_arg_of_callable<F>>>)
         || requires {
             requires std::is_convertible_v<first_arg_of_callable<F>, T>;
             requires !std::is_same_v<typename base_type<T>::element_type,
                                      typename base_type<first_arg_of_callable<F>>::element_type>;
         }
         || requires {
             requires std::is_pointer_v<first_arg_of_callable<F>>;
             requires std::is_base_of_v<typename base_type<T>::element_type,
                                        std::remove_pointer_t<first_arg_of_callable<F>>>;
         }
     ))
    // member function pointer whose class derives from T's pointee/element_type
    || (std::is_member_function_pointer_v<F>
        && std::is_invocable_v<F, typename member_class_of<F>::type*, ExtraArgs...>
        && (
            (std::is_pointer_v<T>
             && std::is_base_of_v<std::remove_pointer_t<T>, typename member_class_of<F>::type>)
            || requires {
                requires std::is_base_of_v<typename base_type<T>::element_type,
                                           typename member_class_of<F>::type>;
            }
        ));

template<typename Signature>
class CovariantCallback;

template<typename R, typename CovariantArg, typename... Args>
class CovariantCallback<R(CovariantArg, Args...)>
    : public std::function<R(const CovariantArg &, Args...)>
{
    using Base = std::function<R(const CovariantArg &, Args...)>;

public:
    using Base::Base;
    using Base::operator=;
    using Base::operator();
    using Base::operator bool;

    CovariantCallback() = default;

    CovariantCallback(const TakesCovariantArg<CovariantArg, Args...> auto &func) { *this = func; }

    // Covariant dispatch — if constexpr selects cast based on callable kind
    CovariantCallback &operator=(const TakesCovariantArg<CovariantArg, Args...> auto &func)
    {
        Base::operator=([f = func](const CovariantArg & covariant, Args... args) mutable -> R {
            using F = base_type<decltype(func)>;
            if constexpr (std::is_member_function_pointer_v<F>) {
                using C = typename member_class_of<F>::type;
                if constexpr (std::is_pointer_v<CovariantArg>) {
                    return (static_cast<C *>(covariant)->*f)(std::forward<Args>(args)...);
                } else {
                    return (std::static_pointer_cast<C>(covariant).get()->*f)(
                        std::forward<Args>(args)...);
                }
            } else {
                using TF = std::decay_t<first_arg_or<F, CovariantArg>>;
                if constexpr (std::is_pointer_v<CovariantArg>) {
                    return f(static_cast<TF>(covariant), std::forward<Args>(args)...);
                } else if constexpr (std::is_pointer_v<TF>) {
                    return f(
                        std::static_pointer_cast<std::remove_pointer_t<TF>>(covariant).get(),
                        std::forward<Args>(args)...);
                } else {
                    return f(
                        std::static_pointer_cast<typename base_type<TF>::element_type>(covariant),
                        std::forward<Args>(args)...);
                }
            }
        });
        return *this;
    }
};

} // namespace Utils
