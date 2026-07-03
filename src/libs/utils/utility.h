// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <utility>
#include <variant>

namespace Utils {
class FilePath;

enum class IterationPolicy { Stop, Continue };

using FilePathPredicate = std::function<bool(const FilePath &)>;

template <typename T>
class Lazy : public std::variant<T, std::function<T()>>
{
public:
    using std::variant<T, std::function<T()>>::variant;

    T value() const
    {
        if (const T *res = std::get_if<T>(this))
            return *res;
        const std::function<T()> *res = std::get_if<std::function<T()>>(this);
        return (*res)();
    }
};

/// RAII object to save a value, and restore it when the scope is left.
template<typename T>
class ScopedSwap
{
    T oldValue;
    T &ref;

public:
    ScopedSwap(T &var, T newValue)
        : oldValue(newValue)
        , ref(var)
    {
        std::swap(ref, oldValue);
    }

    ~ScopedSwap()
    {
        std::swap(ref, oldValue);
    }
};
using ScopedBoolSwap = ScopedSwap<bool>;

template<typename Enumeration>
[[nodiscard]] constexpr std::underlying_type_t<Enumeration> to_underlying(Enumeration enumeration) noexcept
{
    return static_cast<std::underlying_type_t<Enumeration>>(enumeration);
}

template<typename Type, typename... Arguments>
constexpr auto to_array(Arguments &&...arguments)
{
    return std::array<Type, sizeof...(Arguments)>{std::forward<Arguments>(arguments)...};
}

template<typename Type, typename... Arguments>
constexpr auto to_sorted_array(Arguments &&...arguments)
{
    auto array = to_array<Type>(std::forward<Arguments>(arguments)...);
    // std::sort is not constexpr before C++20, so sort in place ourselves.
    for (std::size_t i = 1; i < array.size(); ++i) {
        Type key = array[i];
        std::size_t j = i;
        while (j > 0 && key < array[j - 1]) {
            array[j] = array[j - 1];
            --j;
        }
        array[j] = key;
    }
    return array;
}

// C++17 stand-in for std::identity (C++20). Returns its argument unchanged.
struct identity
{
    template<typename Type>
    constexpr Type &&operator()(Type &&value) const noexcept
    {
        return std::forward<Type>(value);
    }

    using is_transparent = void;
};

// https://www.cppstories.com/2018/09/visit-variants/
// https://en.cppreference.com/w/cpp/utility/variant/visit
// https://en.cppreference.com/w/cpp/utility/variant/visit2
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Utils
