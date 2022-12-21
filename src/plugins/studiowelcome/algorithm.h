
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>

namespace Utils {

//////// FIND
template<typename C, typename F>
[[nodiscard]] typename std::optional<typename C::value_type> findOptional(const C &container,
                                                                            F function)
{
    auto begin = std::cbegin(container);
    auto end = std::cend(container);

    auto it = std::find_if(begin, end, function);
    return it == end ? std::nullopt : std::make_optional(*it);
}

template<typename C>
[[nodiscard]] bool containsItem(const C &container, const typename C::value_type &item)
{
    auto begin = std::cbegin(container);
    auto end = std::cend(container);

    auto it = std::find(begin, end, item);
    return it == end ? false : true;
}

///////// FILTER
template<typename C, typename T = typename C::value_type>
[[nodiscard]] C filterOut(const C &container, const T &value = T())
{
    C out;
    std::copy_if(std::begin(container), std::end(container), inserter(out), [&](const auto &item) {
        return item != value;
    });
    return out;
}

template<typename C>
[[nodiscard]] C filtered(const C &container)
{
    return filterOut(container, typename C::value_type{});
}

/////// MODIFY
template<typename SC, typename C>
void concat(C &out, const SC &container)
{
    std::copy(std::begin(container), std::end(container), inserter(out));
}

template<typename C, typename T>
bool erase_one(C &container, const T &value)
{
    typename C::const_iterator i = std::find(std::cbegin(container), std::cend(container), value);
    if (i == std::cend(container))
        return false;

    container.erase(i);
    return true;
}

template<typename C, typename T>
void prepend(C &container, const T &value)
{
    container.insert(std::cbegin(container), value);
}

/////// OTHER
template<typename RC, typename SC>
[[nodiscard]] RC flatten(const SC &container)
{
    RC result;

    for (const auto &innerContainer : container)
        concat(result, innerContainer);

    return result;
}

template<template<typename, typename...> class C,
         typename T,
         typename... TArgs,
         typename RT = typename std::decay_t<T>::value_type,
         typename RC = C<RT>>
[[nodiscard]] auto flatten(const C<T, TArgs...> &container)
{
    using SC = C<T, TArgs...>;
    return flatten<RC, SC>(container);
}

} // namespace Utils
