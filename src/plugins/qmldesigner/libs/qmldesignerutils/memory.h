// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>
#include <tuple>

namespace QmlDesigner {

template<typename Type, typename ResultType, typename... Arguments>
class LazySharedPtr
{

public:
    LazySharedPtr(Arguments &&...arguments)
        : m_arguments{std::forward_as_tuple(std::forward<Arguments>(arguments)...)}
    {}

    operator std::shared_ptr<ResultType>() const
    {
        return std::apply(
            [](auto &&...arguments) {
                return std::make_shared<Type>(std::forward<decltype(arguments)>(arguments)...);
            },
            m_arguments);
    }

private:
    std::tuple<Arguments...> m_arguments;
};

template<typename Type, typename ResultType, typename... Arguments>
LazySharedPtr<Type, ResultType, Arguments...> makeLazySharedPtr(Arguments &&...arguments)
{
    return LazySharedPtr<Type, ResultType, Arguments...>(std::forward<Arguments>(arguments)...);
}

} // namespace QmlDesigner
