/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
template <typename Type>
auto equalTo(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of equalTo should be the same to prevent "
                      "unnecessary conversion.");
        return entry == value;
    };
}

template <typename Type>
auto unequalTo(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of unequalTo should be the same to prevent "
                      "unnecessary conversion.");
        return !(entry == value);
    };
}

template <typename Type>
auto lessThan(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of unequalTo should be the same to prevent "
                      "unnecessary conversion.");
        return entry < value;
    };
}

template <typename Type>
auto lessEqualThan(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of lessEqualThan should be the same to "
                      "prevent unnecessary conversion.");
        return !(value < entry);
    };
}

template <typename Type>
auto greaterThan(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of greaterThan should be the same to "
                      "prevent unnecessary conversion.");
        return value < entry;
    };
}

template <typename Type>
auto greaterEqualThan(Type &&value)
{
    return [value = std::forward<Type>(value)] (const auto &entry)
    {
        static_assert(std::is_same<std::decay_t<Type>,
                      std::decay_t<decltype(entry)>>::value,
                      "The container and predicate type of greaterEqualThan should be the same to "
                      "prevent unnecessary conversion.");
        return !(entry < value);
    };
}
} // namespace Utils
