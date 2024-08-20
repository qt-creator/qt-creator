// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

namespace QmlDesigner {

template<typename Value, typename Projection = std::identity>
auto compare_to(auto compare, Value &&value, Projection projection = {})
{
    return [compare = std::move(compare),
            projection = std::move(projection),
            compareValue = std::forward<Value>(value)]<typename Value2>(Value2 &&value) {
        return std::invoke(compare, compareValue, std::invoke(projection, std::forward<Value2>(value)));
    };
}

template<typename Value, typename Projection = std::identity>
auto is_equal_to(Value &&value, Projection projection = {})
{
    return compare_to(std::ranges::equal_to{}, std::forward<Value>(value), std::move(projection));
}

template<typename Projection1 = std::identity, typename Projection2 = std::identity>
auto compare_with(auto compare, Projection1 projection1 = {}, Projection1 projection2 = {})
{
    return [compare = std::move(compare),
            projection1 = std::move(projection1),
            projection2 = std::move(projection2)]<typename Value1, typename Value2>(Value1 &&value1,
                                                                                    Value2 &&value2) {
        return std::invoke(compare,
                           std::invoke(projection1, std::forward<Value1>(value1)),
                           std::invoke(projection2, std::forward<Value2>(value2)));
    };
}

template<typename Projection = std::identity>
auto logical(auto compare, Projection projection = {})
{
    return [=](auto &&value) {
        return std::invoke(compare, std::invoke(projection, std::forward<decltype(value)>(value)));
    };
}

template<typename Projection = std::identity>
auto is_null(Projection projection = {})
{
    return logical(std::logical_not{}, projection);
}

} // namespace QmlDesigner
