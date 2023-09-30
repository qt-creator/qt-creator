// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Utils {

namespace Internal {
template<typename Type, std::size_t size, std::size_t... index>
constexpr std::array<std::remove_cv_t<Type>, size> to_array_implementation(
    Type (&&array)[size], std::index_sequence<index...>)
{
    return {{std::move(array[index])...}};
}
} // namespace Internal

template<typename Type, std::size_t size>
constexpr std::array<std::remove_cv_t<Type>, size> to_array(Type (&&array)[size])
{
    return Internal::to_array_implementation(std::move(array), std::make_index_sequence<size>{});
}

} // namespace Utils
