// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <functional>
#include <tuple>

namespace QmlDesigner {

inline constexpr auto makeCompare = [](auto... projections) {
    return [=](auto compare) {
        return [=](const auto &first, const auto &second) {
            return compare(std::forward_as_tuple(std::invoke(projections, first)...),
                           std::forward_as_tuple(std::invoke(projections, second)...));
        };
    };
};

inline constexpr auto makeLess = [](auto... projections) {
    return [=](const auto &first, const auto &second) {
        return std::ranges::less(std::forward_as_tuple(std::invoke(projections, first)...),
                                 std::forward_as_tuple(std::invoke(projections, second)...));
    };
};

inline constexpr auto makeEqual = [](auto... projections) {
    return [=](const auto &first, const auto &second) {
        return std::ranges::equal_to(std::forward_as_tuple(std::invoke(projections, first)...),
                                     std::forward_as_tuple(std::invoke(projections, second)...));
    };
};

template<class Function, class Argument>
inline constexpr auto bind_back(Function &&function, Argument &&argument)
{
    return std::bind(function, std::placeholders::_1, std::forward<Argument>(argument));
}
} // namespace QmlDesigner
