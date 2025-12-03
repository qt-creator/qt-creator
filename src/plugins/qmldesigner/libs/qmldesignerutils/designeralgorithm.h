// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <iterator>
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

template<typename Container>
concept has_reserve = requires(Container c) { c.reserve(0); };

template<template<typename...> typename Container, std::ranges::view View>
constexpr auto to(View &&view, std::integral auto reserve)
{
    using ContainerType = Container<std::iter_value_t<std::ranges::iterator_t<View>>>;
    ContainerType container;

    if constexpr (has_reserve<ContainerType>)
        container.reserve(static_cast<std::ranges::range_size_t<ContainerType>>(reserve));

    std::ranges::copy(view, std::back_inserter(container));

    return container;
}

template<typename Container, std::ranges::view View>
constexpr auto to(View &&view, std::integral auto reserve)
{
    Container container;

    if constexpr (has_reserve<Container>)
        container.reserve(static_cast<std::ranges::range_size_t<Container>>(reserve));

    std::ranges::copy(view, std::back_inserter(container));

    return container;
}

template<typename Type, std::size_t size, std::ranges::view View>
constexpr auto toDefaultInitializedArray(View &&view)
{
    std::array<Type, size> container{};

    std::ranges::copy(view | std::views::take(size), container.begin());

    return container;
}

} // namespace QmlDesigner::CoreUtils
