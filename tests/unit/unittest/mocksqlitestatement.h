/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <googletest.h>

#include <sqlitebasestatement.h>


class BaseMockSqliteStatement
{
public:
    MOCK_METHOD0(next, bool ());
    MOCK_METHOD0(step, void ());
    MOCK_METHOD0(execute, void ());
    MOCK_METHOD0(reset, void ());

    MOCK_CONST_METHOD1(fetchIntValue, int (int));
    MOCK_CONST_METHOD1(fetchLongValue, long (int));
    MOCK_CONST_METHOD1(fetchLongLongValue, long long (int));
    MOCK_CONST_METHOD1(fetchDoubleValue, double (int));
    MOCK_CONST_METHOD1(fetchSmallStringValue, Utils::SmallString (int));
    MOCK_CONST_METHOD1(fetchPathStringValue, Utils::PathString (int));

    template<typename Type>
    Type fetchValue(int column) const;

    MOCK_METHOD2(bind, void (int, int));
    MOCK_METHOD2(bind, void (int, long long));
    MOCK_METHOD2(bind, void (int, double));
    MOCK_METHOD2(bind, void (int, Utils::SmallStringView));
    MOCK_METHOD2(bind, void (int, long));

    MOCK_METHOD1(prepare, void (Utils::SmallStringView sqlStatement));
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

class MockSqliteStatement : public Sqlite::StatementImplementation<NiceMock<BaseMockSqliteStatement>>
{
public:
    explicit MockSqliteStatement()
        : Sqlite::StatementImplementation<NiceMock<BaseMockSqliteStatement>>()
    {}


protected:
    void checkIsWritableStatement();
};
