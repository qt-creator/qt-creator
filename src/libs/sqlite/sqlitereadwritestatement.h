/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
        ImmediateTransaction transaction{Base::database()};

        auto resultValue = Base::template value<ResultType>(queryValues...);

        transaction.commit();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto optionalValueWithTransaction(const QueryTypes &...queryValues)
    {
        ImmediateTransaction transaction{Base::database()};

        auto resultValue = Base::template optionalValue<ResultType>(queryValues...);

        transaction.commit();

        return resultValue;
    }

    template<typename ResultType, typename... QueryTypes>
    auto valuesWithTransaction(std::size_t reserveSize, const QueryTypes &...queryValues)
    {
        ImmediateTransaction transaction{Base::database()};

        auto resultValues = Base::template values<ResultType>(reserveSize, queryValues...);

        transaction.commit();

        return resultValues;
    }

    template<typename Callable, typename... QueryTypes>
    void readCallbackWithTransaction(Callable &&callable, const QueryTypes &...queryValues)
    {
        ImmediateTransaction transaction{Base::database()};

        Base::readCallback(std::forward<Callable>(callable), queryValues...);

        transaction.commit();
    }

    template<typename Container, typename... QueryTypes>
    void readToWithTransaction(Container &container, const QueryTypes &...queryValues)
    {
        ImmediateTransaction transaction{Base::database()};

        Base::readTo(container, queryValues...);

        transaction.commit();
    }

    void executeWithTransaction()
    {
        ImmediateTransaction transaction{Base::database()};

        Base::execute();

        transaction.commit();
    }
};

} // namespace Sqlite
