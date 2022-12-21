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
        DeferredTransaction transaction{Base::database()};

        auto resultValue = Base::template value<ResultType>(queryValues...);

        transaction.commit();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        DeferredTransaction transaction{Base::database()};

        auto resultValue = Base::template optionalValue<ResultType>(queryValues...);

        transaction.commit();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto valuesWithTransaction(std::size_t reserveSize, const QueryTypes &...queryValues)
    {
        DeferredTransaction transaction{Base::database()};

        auto resultValues = Base::template values<ResultType>(reserveSize, queryValues...);

        transaction.commit();

        return resultValues;
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable, const QueryTypes &...queryValues)
    {
        DeferredTransaction transaction{Base::database()};

        Base::readCallback(std::forward<Callable>(callable), queryValues...);

        transaction.commit();
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container, const QueryTypes &...queryValues)
    {
        DeferredTransaction transaction{Base::database()};

        Base::readTo(container, queryValues...);

        transaction.commit();
    }

protected:
    void checkIsReadOnlyStatement()
    {
        if (!Base::isReadOnlyStatement())
            throw NotReadOnlySqlStatement(
                "SqliteStatement::SqliteReadStatement: is not read only statement!");
    }
};

template<int ResultCount>
ReadStatement(ReadStatement<ResultCount> &) -> ReadStatement<ResultCount>;
template<int ResultCount>
ReadStatement(const ReadStatement<ResultCount> &) -> ReadStatement<ResultCount>;

} // namespace Sqlite
