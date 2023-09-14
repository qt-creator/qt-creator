// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitebasestatement.h"

namespace Sqlite {

template<int ResultCount, int BindParameterCount = 0>
class ReadStatement final
    : protected StatementImplementation<BaseStatement, ResultCount, BindParameterCount>
{
    using Base = StatementImplementation<BaseStatement, ResultCount, BindParameterCount>;

public:
    ReadStatement(Utils::SmallStringView sqlStatement, Database &database)
        : Base{sqlStatement, database}
    {
        checkIsReadOnlyStatement();
        Base::checkBindingParameterCount(BindParameterCount);
        Base::checkColumnCount(ResultCount);
    }

    using Base::optionalValue;
    using Base::range;
    using Base::rangeWithTransaction;
    using Base::readCallback;
    using Base::readTo;
    using Base::toValue;
    using Base::value;
    using Base::values;

    template<typename ResultType, typename... QueryTypes>
    auto valueWithTransaction(const QueryTypes &...queryValues)
    {
        return withImplicitTransaction(Base::database(), [&] {
            return Base::template value<ResultType>(queryValues...);
        });
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        return withImplicitTransaction(Base::database(), [&] {
            return Base::template optionalValue<ResultType>(queryValues...);
        });
    }

    template<typename ResultType, std::size_t capacity = 32, typename... QueryTypes>
    auto valuesWithTransaction(const QueryTypes &...queryValues)
    {
        return withImplicitTransaction(Base::database(), [&] {
            return Base::template values<ResultType, capacity>(queryValues...);
        });
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable, const QueryTypes &...queryValues)
    {
        withImplicitTransaction(Base::database(), [&] {
            Base::readCallback(std::forward<Callable>(callable), queryValues...);
        });
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container, const QueryTypes &...queryValues)
    {
        withImplicitTransaction(Base::database(), [&] { Base::readTo(container, queryValues...); });
    }

protected:
    void checkIsReadOnlyStatement()
    {
        if (!Base::isReadOnlyStatement())
            throw NotReadOnlySqlStatement();
    }
};

template<int ResultCount>
ReadStatement(ReadStatement<ResultCount> &) -> ReadStatement<ResultCount>;
template<int ResultCount>
ReadStatement(const ReadStatement<ResultCount> &) -> ReadStatement<ResultCount>;

} // namespace Sqlite
