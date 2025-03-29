// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace Utils
{

//////////////////
// find helpers
//////////////////
template<typename R, typename S, typename T>
decltype(auto) equal(R (S::*function)() const, T value)
{
    // This should use std::equal_to<> instead of std::equal_to<T>,
    // but that's not supported everywhere yet, since it is C++14
    return std::bind<bool>(std::equal_to<T>(), value, std::bind(function, std::placeholders::_1));
}

template<typename R, typename S, typename T>
decltype(auto) equal(R S::*member, T value)
{
    return std::bind<bool>(std::equal_to<T>(), value, std::bind(member, std::placeholders::_1));
}

//////////////////
// comparison predicates
//////////////////

template<typename Type, typename Compare, typename Projection>
auto compare(Type &&value, Compare compare, Projection projection)
{
    if constexpr (std::is_lvalue_reference_v<Type>) {
        return [&value, compare, projection](const auto &entry) {
            return std::invoke(compare, value, std::invoke(projection, entry));
        };
    } else {
        return [value = std::forward<Type>(value), compare, projection](const auto &entry) {
            return std::invoke(compare, value, std::invoke(projection, entry));
        };
    }
}

template<typename Type, typename Projection = std::identity>
auto equalTo(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::equal_to{}, projection);
}

template<typename Type, typename Projection = std::identity>
auto unequalTo(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::not_equal_to{}, projection);
}

template<typename Type, typename Projection = std::identity>
auto lessThan(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::less{}, projection);
}

template<typename Type, typename Projection = std::identity>
auto lessEqualThan(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::less_equal{}, projection);
}

template<typename Type, typename Projection = std::identity>
auto greaterThan(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::greater{}, projection);
}

template<typename Type, typename Projection = std::identity>
auto greaterEqualThan(Type &&value, Projection projection = {})
{
    return compare(value, std::ranges::greater_equal{}, projection);
}
} // namespace Utils
