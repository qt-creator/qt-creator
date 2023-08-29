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
    ReadWriteStatement(Utils::SmallStringView sqlStatement, Database &database)
        : Base{sqlStatement, database}
    {
        Base::checkBindingParameterCount(BindParameterCount);
        Base::checkColumnCount(ResultCount);
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
    auto valueWithTransaction(const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(Base::database(), [&] {
            return Base::template value<ResultType>(queryValues...);
        });
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(Base::database(), [&] {
            return Base::template optionalValue<ResultType>(queryValues...);
        });
    }

    template<typename ResultType,
             std::size_t capacity = 32,
             typename... QueryTypes>
    auto valuesWithTransaction(const QueryTypes &...queryValues)
    {
        return withImmediateTransaction(Base::database(), [&] {
            return Base::template values<ResultType, capacity>(queryValues...);
        });
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable, const QueryTypes &...queryValues)
    {
        withImmediateTransaction(Base::database(), [&] {
            Base::readCallback(std::forward<Callable>(callable), queryValues...);
        });
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container, const QueryTypes &...queryValues)
    {
        withImmediateTransaction(Base::database(), [&] {
            Base::readTo(container, queryValues...);
        });
    }

    void executeWithTransaction()
    {
        withImmediateTransaction(Base::database(), [&] {
            Base::execute();
        });
    }
};

} // namespace Sqlite
