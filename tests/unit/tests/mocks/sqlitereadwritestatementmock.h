// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorageids.h>

#include <sqlite/sqlitevalue.h>
#include <utils/smallstring.h>

#include <cstdint>
#include <optional>
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
    auto optionalValue([[maybe_unused]] const QueryTypes &...queryValues)
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
