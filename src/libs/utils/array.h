// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <array>

namespace Utils {

template<typename Type, typename... Arguments>
constexpr auto to_array(Arguments &&...arguments)
{
    return std::array<Type, sizeof...(Arguments)>{std::forward<Arguments>(arguments)...};
}

template<typename Type, typename... Arguments>
constexpr auto to_sorted_array(Arguments &&...arguments)
{
    auto array = to_array<Type>(std::forward<Arguments>(arguments)...);
    std::ranges::sort(array);
    return array;
}

} // namespace Utils
