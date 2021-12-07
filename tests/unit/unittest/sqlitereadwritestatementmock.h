/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "googletest.h"

#include <projectstorageids.h>

#include <sqlite/sqlitevalue.h>
#include <utils/optional.h>
#include <utils/smallstring.h>

#include <cstdint>
#include <tuple>
#include <vector>

class SqliteDatabaseMock;

class SqliteReadWriteStatementMockBase
{
public:
    SqliteReadWriteStatementMockBase() = default;
    SqliteReadWriteStatementMockBase(Utils::SmallStringView sqlStatement,
                                     SqliteDatabaseMock &databaseMock);

    MOCK_METHOD(QmlDesigner::TypeId,
                valueReturnsTypeId,
                (Utils::SmallStringView name, long long id, long long),
                ());
    MOCK_METHOD(QmlDesigner::TypeId,
                valueReturnsTypeId,
                (Utils::SmallStringView name, long long id, long long, long long),
                ());
    MOCK_METHOD(QmlDesigner::TypeId, valueReturnsTypeId, (Utils::SmallStringView name), ());
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                valueReturnsPropertyDeclarationId,
                (long long, Utils::SmallStringView, long long),
                ());
    MOCK_METHOD(QmlDesigner::PropertyDeclarationId,
                valueReturnsPropertyDeclarationId,
                (long long, Utils::SmallStringView, long long, int),
                ());

    template<typename ResultType, typename... QueryTypes>
    auto optionalValue(const QueryTypes &...queryValues)
    {
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

    template<typename ResultType, typename... QueryTypes>
    auto value(const QueryTypes &...queryValues)
    {
        if constexpr (std::is_same_v<ResultType, QmlDesigner::TypeId>)
            return valueReturnsTypeId(queryValues...);
        else if constexpr (std::is_same_v<ResultType, QmlDesigner::PropertyDeclarationId>)
            return valueReturnsPropertyDeclarationId(queryValues...);
        else
            static_assert(!std::is_same_v<ResultType, ResultType>,
                          "SqliteReadStatementMock::value does not handle result type!");
    }

public:
    Utils::SmallString sqlStatement;
};

template<int ResultCount, int BindParameterCount = 0>
class SqliteReadWriteStatementMock : public SqliteReadWriteStatementMockBase
{
public:
    using SqliteReadWriteStatementMockBase::SqliteReadWriteStatementMockBase;
};
