// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <ranges>

namespace QmlDesigner::CoreUtils {

template<typename Element, typename... Elements>
#ifdef Q_CC_MSVC
__forceinline
#else
[[gnu::always_inline]]
#endif
    constexpr bool
    contains(Element element, Elements... elements)
{
    return ((element == elements) || ...);
}

template<template<typename...> typename Container, std::ranges::view View>
constexpr auto to(View &&view)
{
    return Container<std::iter_value_t<std::ranges::iterator_t<View>>>(std::ranges::begin(view),
                                                                       std::ranges::end(view));
}

template<typename Container, std::ranges::view View>
constexpr auto to(View &&view)
{
    return Container(std::ranges::begin(view), std::ranges::end(view));
}

} // namespace QmlDesigner::CoreUtils
