// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>
#include <tuple>

namespace QmlDesigner {

template<typename Pointer, typename... Arguments>
class LazyPtr
{
    using ResultType = Pointer::element_type;

public:
    LazyPtr(Arguments &&...arguments)
        : m_arguments{std::forward_as_tuple(std::forward<Arguments>(arguments)...)}
    {}

    operator Pointer() const
    {
        return std::apply(
            [](auto &&...arguments) {
                return Pointer(new ResultType(std::forward<decltype(arguments)>(arguments)...));
            },
            m_arguments);
    }

private:
    std::tuple<Arguments...> m_arguments;
};

template<typename ResultType, typename... Arguments>
using LazyUniquePtr = LazyPtr<std::unique_ptr<ResultType>, Arguments...>;

template<typename Result, typename... Arguments>
LazyUniquePtr<Result, Arguments...> makeLazyUniquePtr(Arguments &&...arguments)
{
    return LazyUniquePtr<Result, Arguments...>(std::forward<Arguments>(arguments)...);
}

template<typename ResultType, typename... Arguments>
using LazySharedPtr = LazyPtr<std::shared_ptr<ResultType>, Arguments...>;

template<typename ResultType, typename... Arguments>
LazySharedPtr<ResultType, Arguments...> makeLazySharedPtr(Arguments &&...arguments)
{
    return LazySharedPtr<ResultType, Arguments...>(std::forward<Arguments>(arguments)...);
}

} // namespace QmlDesigner
