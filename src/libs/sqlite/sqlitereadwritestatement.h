// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitebasestatement.h"

namespace Sqlite {

template<int ResultCount = 0, int BindParameterCount = 0>
class ReadWriteStatement final
    : protected StatementImplementation<BaseStatement, ResultCount, BindParameterCount>
{
    friend class DatabaseBackend;
    using Base = StatementImplementation<BaseStatement, ResultCount, BindParameterCount>;

public:
    ReadWriteStatement(Utils::SmallStringView sqlStatement,
                       Database &database,
                       const source_location &sourceLocation = source_location::current())
        : Base{sqlStatement, database, sourceLocation}
    {
        Base::checkBindingParameterCount(BindParameterCount, sourceLocation);
        Base::checkColumnCount(ResultCount, sourceLocation);
    }

    using Base::execute;
    using Base::optionalValue;
    using Base::readCallback;
    using Base::readTo;
    using Base::toValue;
    using Base::value;
    using Base::values;
    using Base::write;

    template<typename ResultType, typename... QueryTypes>
    auto valueWithTransaction(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(
            Base::database(),
            [&] { return Base::template value<ResultType>(sourceLocation, queryValues...); },
            sourceLocation);
    }

    template<typename ResultType, typename... QueryTypes>
    auto valueWithTransaction(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return valueWithTransaction<ResultType>(sourceLocation, queryValues...);
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const source_location &sourceLocation,
                                      const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(
            Base::database(),
            [&] { return Base::template optionalValue<ResultType>(sourceLocation, queryValues...); },
            sourceLocation);
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return optionalValueWithTransaction<ResultType>(sourceLocation, queryValues...);
    }

    template<typename ResultType, std::size_t capacity = 32, typename... QueryTypes>
    auto valuesWithTransaction(const source_location &sourceLocation, const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(
            Base::database(),
            [&] {
                return Base::template values<ResultType, capacity>(sourceLocation, queryValues...);
            },
            sourceLocation);
    }

    template<typename ResultType, std::size_t capacity = 32, typename... QueryTypes>
    auto valuesWithTransaction(const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        return valuesWithTransaction<ResultType, capacity>(sourceLocation, queryValues...);
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable,
                                     const source_location &sourceLocation,
                                     const QueryTypes &...queryValues)
    {
        withImmediateTransaction(
            Base::database(),
            [&] {
                Base::readCallback(std::forward<Callable>(callable), sourceLocation, queryValues...);
            },
            sourceLocation);
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable, const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        readCallbackWithTransaction(callable, sourceLocation, queryValues...);
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container,
                               const source_location &sourceLocation,
                               const QueryTypes &...queryValues)
    {
        withImmediateTransaction(
            Base::database(),
            [&] { Base::readTo(container, sourceLocation, queryValues...); },
            sourceLocation);
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container, const QueryTypes &...queryValues)
    {
        static constexpr auto sourceLocation = source_location::current();
        readToWithTransaction(container, sourceLocation, queryValues...);
    }

    void executeWithTransaction(const source_location &sourceLocation = source_location::current())
    {
        withImmediateTransaction(Base::database(), [&] { Base::execute(sourceLocation); }, sourceLocation);
    }
};

} // namespace Sqlite
