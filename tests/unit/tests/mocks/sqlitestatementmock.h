// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <sqlitebasestatement.h>

class BaseSqliteStatementMock
{
public:
    MOCK_METHOD(bool, next, ());
    MOCK_METHOD(void, step, ());
    MOCK_METHOD(void, reset, ());

    MOCK_METHOD(int, fetchIntValue, (int), (const));
    MOCK_METHOD(long, fetchLongValue, (int), (const));
    MOCK_METHOD(long long, fetchLongLongValue, (int), (const));
    MOCK_METHOD(double, fetchDoubleValue, (int), (const));
    MOCK_METHOD(Utils::SmallString, fetchSmallStringValue, (int), (const));
    MOCK_METHOD(Utils::PathString, fetchPathStringValue, (int), (const));

    template<typename Type>
    Type fetchValue(int column) const;

    MOCK_METHOD(void, bind, (int, int), ());
    MOCK_METHOD(void, bind, (int, long long), ());
    MOCK_METHOD(void, bind, (int, double), ());
    MOCK_METHOD(void, bind, (int, Utils::SmallStringView), ());
    MOCK_METHOD(void, bind, (int, long) );
    MOCK_METHOD(int, bindingIndexForName, (Utils::SmallStringView name), (const));

    MOCK_METHOD(void, prepare, (Utils::SmallStringView sqlStatement));
};

template<>
int BaseSqliteStatementMock::fetchValue<int>(int column) const
{
    return fetchIntValue(column);
}

template<>
long BaseSqliteStatementMock::fetchValue<long>(int column) const
{
    return fetchLongValue(column);
}

template<>
long long BaseSqliteStatementMock::fetchValue<long long>(int column) const
{
    return fetchLongLongValue(column);
}

template<>
double BaseSqliteStatementMock::fetchValue<double>(int column) const
{
    return fetchDoubleValue(column);
}

template<>
Utils::SmallString BaseSqliteStatementMock::fetchValue<Utils::SmallString>(int column) const
{
    return fetchSmallStringValue(column);
}

template<>
Utils::PathString BaseSqliteStatementMock::fetchValue<Utils::PathString>(int column) const
{
    return fetchPathStringValue(column);
}

class SqliteStatementMock : public Sqlite::StatementImplementation<NiceMock<BaseSqliteStatementMock>>
{
public:
    explicit SqliteStatementMock()
        : Sqlite::StatementImplementation<NiceMock<BaseSqliteStatementMock>>()
    {}


protected:
    void checkIsWritableStatement();
};
