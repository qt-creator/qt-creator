// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"
#include "sqlitedatabasemock.h"

#include <sqlitebasestatement.h>


class BaseMockSqliteStatement
{
public:
    using Database = SqliteDatabaseMock;

    BaseMockSqliteStatement() = default;
    BaseMockSqliteStatement(SqliteDatabaseMock &databaseMock)
        : m_databaseMock{&databaseMock}
    {}

    MOCK_METHOD0(next, bool());
    MOCK_METHOD0(step, void ());
    MOCK_METHOD0(reset, void ());

    MOCK_CONST_METHOD1(fetchIntValue, int (int));
    MOCK_CONST_METHOD1(fetchLongValue, long (int));
    MOCK_CONST_METHOD1(fetchLongLongValue, long long (int));
    MOCK_CONST_METHOD1(fetchDoubleValue, double (int));
    MOCK_CONST_METHOD1(fetchSmallStringValue, Utils::SmallString(int));
    MOCK_CONST_METHOD1(fetchSmallStringViewValue, Utils::SmallStringView(int));
    MOCK_CONST_METHOD1(fetchPathStringValue, Utils::PathString (int));
    MOCK_CONST_METHOD1(fetchValueView, Sqlite::ValueView(int));

    template<typename Type>
    Type fetchValue(int column) const;

    MOCK_METHOD2(bind, void (int, int));
    MOCK_METHOD2(bind, void (int, long long));
    MOCK_METHOD2(bind, void (int, double));
    MOCK_METHOD2(bind, void (int, Utils::SmallStringView));
    MOCK_METHOD2(bind, void (int, long));

    MOCK_METHOD1(prepare, void(Utils::SmallStringView sqlStatement));

    MOCK_METHOD1(checkColumnCount, void(int));
    MOCK_METHOD1(checkBindingParameterCount, void(int));

    MOCK_CONST_METHOD0(isReadOnlyStatement, bool());

    SqliteDatabaseMock &database() { return *m_databaseMock; }

private:
    SqliteDatabaseMock *m_databaseMock = nullptr;
};

template<>
int BaseMockSqliteStatement::fetchValue<int>(int column) const
{
    return fetchIntValue(column);
}

template<>
long BaseMockSqliteStatement::fetchValue<long>(int column) const
{
    return fetchLongValue(column);
}

template<>
long long BaseMockSqliteStatement::fetchValue<long long>(int column) const
{
    return fetchLongLongValue(column);
}

template<>
double BaseMockSqliteStatement::fetchValue<double>(int column) const
{
    return fetchDoubleValue(column);
}

template<>
Utils::SmallString BaseMockSqliteStatement::fetchValue<Utils::SmallString>(int column) const
{
    return fetchSmallStringValue(column);
}

template<>
Utils::PathString BaseMockSqliteStatement::fetchValue<Utils::PathString>(int column) const
{
    return fetchPathStringValue(column);
}

template<int ResultCount = 1, int BindParameterCount = 0>
class MockSqliteStatement
    : public Sqlite::StatementImplementation<BaseMockSqliteStatement, ResultCount, BindParameterCount>
{
    using Base = Sqlite::StatementImplementation<BaseMockSqliteStatement, ResultCount, BindParameterCount>;

public:
    explicit MockSqliteStatement(SqliteDatabaseMock &databaseMock)
        : Base{databaseMock}
    {}

protected:
    void checkIsWritableStatement();
};
