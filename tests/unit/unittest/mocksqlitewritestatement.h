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

#include "googletest.h"

#include <utils/smallstring.h>

class MockSqliteDatabase;

class MockSqliteWriteStatement
{
public:
    MockSqliteWriteStatement() = default;
    MockSqliteWriteStatement(Utils::SmallStringView sqlStatement, MockSqliteDatabase &)
        : sqlStatement(sqlStatement)
    {}

    MOCK_METHOD0(execute,
                 void ());

    MOCK_METHOD2(bind, void(int, Utils::SmallStringView));

    MOCK_METHOD2(bindValues,
                 void (Utils::SmallStringView, Utils::SmallStringView));

    MOCK_METHOD4(write,
                 void (uint, Utils::SmallStringView, Utils::SmallStringView, uint));

    MOCK_METHOD4(write,
                 void (uint, uint, uint, uint));

    MOCK_METHOD4(write,
                 void (long long, int, int, int));

    MOCK_METHOD5(write,
                 void (long long, int, int, int, int));

    MOCK_METHOD2(write, void(uint, Utils::SmallStringView));

    MOCK_METHOD2(write, void(int, Utils::SmallStringView));

    MOCK_METHOD2(write, void(Utils::SmallStringView, Utils::SmallStringView));

    MOCK_METHOD3(write, void(int, Utils::SmallStringView, long long));

    MOCK_METHOD3(write,
                 void (Utils::SmallStringView, Utils::SmallStringView, Utils::SmallStringView));

    MOCK_METHOD4(write,
                 void (Utils::SmallStringView,
                       Utils::SmallStringView,
                       Utils::SmallStringView,
                       Utils::SmallStringView));

    MOCK_METHOD8(write,
                 void(int,
                      Utils::SmallStringView,
                      Utils::SmallStringView,
                      Utils::SmallStringView,
                      Utils::SmallStringView,
                      int,
                      int,
                      int));

    MOCK_METHOD1(write,
                 void (Utils::SmallStringView));

    MOCK_METHOD1(write,
                 void (long long));

    MOCK_METHOD1(write,
                 void (int));

    MOCK_METHOD2(write, void(int, long long));

    MOCK_METHOD2(write,
                 void (int, int));

    MOCK_METHOD3(write,
                 void (uint, uint, uint));

    MOCK_METHOD3(write, void(int, off_t, time_t));

    MOCK_METHOD2(write,
                 void (uint, uint));

    MOCK_METHOD2(write,
                 void (uchar, int));
    MOCK_METHOD4(write, void(int, int, uchar, uchar));
    MOCK_METHOD2(write,
                 void (long long, int));
    MOCK_METHOD2(write, void(long long, Utils::SmallStringView));
    Utils::SmallString sqlStatement;
};
