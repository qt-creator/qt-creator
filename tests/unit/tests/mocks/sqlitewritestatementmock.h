// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <sqliteblob.h>
#include <sqlitevalue.h>

class SqliteDatabaseMock;

class SqliteWriteStatementMockBase
{
public:
    SqliteWriteStatementMockBase() = default;
    SqliteWriteStatementMockBase(Utils::SmallStringView sqlStatement, SqliteDatabaseMock &database);

    MOCK_METHOD(void, execute, (), ());

    MOCK_METHOD(void, write, (Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (long long), ());
    MOCK_METHOD(void, write, (long long, long long), ());
    MOCK_METHOD(void, write, (long long, long long, int), ());
    MOCK_METHOD(void, write, (long long, long long, long long), ());
    MOCK_METHOD(void, write, (long long, unsigned int), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, long long), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (long long, Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, int, int, long long), ());
    MOCK_METHOD(void,
                write,
                (Utils::SmallStringView, Utils::SmallStringView, Utils::SmallStringView),
                ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, Utils::SmallStringView, long long), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, Utils::SmallStringView, double), ());
    MOCK_METHOD(void, write, (long long, Utils::SmallStringView, Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (long long, Utils::SmallStringView, const Sqlite::Value &), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, long long, Sqlite::BlobView), ());
    MOCK_METHOD(void, write, (Utils::SmallStringView, long long, Sqlite::BlobView, Sqlite::BlobView), ());
    MOCK_METHOD(void,
                write,
                (Utils::SmallStringView, long long, Sqlite::BlobView, Sqlite::BlobView, Sqlite::BlobView),
                ());
    MOCK_METHOD(void,
                write,
                (Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView),
                ());
    MOCK_METHOD(void, write, (long long, Utils::SmallStringView, long long, int), ());
    MOCK_METHOD(void,
                write,
                (long long, Utils::SmallStringView, Utils::SmallStringView, Utils::SmallStringView),
                ());
    MOCK_METHOD(void, write, (long long, long long, Utils::SmallStringView, Utils::SmallStringView), ());
    MOCK_METHOD(void,
                write,
                (Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView),
                ());
    MOCK_METHOD(void,
                write,
                (int,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 Utils::SmallStringView,
                 int,
                 int,
                 int),
                ());

    MOCK_METHOD(void, write, (void *, long long), ());
    MOCK_METHOD(void, write, (int), ());
    MOCK_METHOD(void, write, (int, long long), ());
    MOCK_METHOD(void, write, (int, long long, long long), ());
    MOCK_METHOD(void, write, (int, int), ());
    MOCK_METHOD(void, write, (uint, uint, uint), ());
    MOCK_METHOD(void, write, (int, off_t, time_t), ());
    MOCK_METHOD(void, write, (uint, uint), ());
    MOCK_METHOD(void, write, (uchar, int), ());
    MOCK_METHOD(void, write, (int, int, uchar, uchar), ());
    MOCK_METHOD(void, write, (long long, int), ());
    MOCK_METHOD(void, write, (uint, Utils::SmallStringView, Utils::SmallStringView, uint), ());
    MOCK_METHOD(void, write, (uint, uint, uint, uint), ());
    MOCK_METHOD(void, write, (long long, int, int, int), ());
    MOCK_METHOD(void, write, (long long, int, int, int, int), ());
    MOCK_METHOD(void, write, (uint, Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (int, Utils::SmallStringView), ());
    MOCK_METHOD(void, write, (int, Utils::SmallStringView, long long), ());
    MOCK_METHOD(void, write, (long long, Sqlite::NullValue, Sqlite::NullValue), ());

    MOCK_METHOD(void, write, (Utils::span<int>, Utils::span<long long>), ());
    MOCK_METHOD(void, write, (Utils::span<long long>), ());
    MOCK_METHOD(void, write, (Utils::span<int>), ());

    Utils::SmallString sqlStatement;
};

template<int BindParameterCount = 0>
class SqliteWriteStatementMock : public SqliteWriteStatementMockBase
{
public:
    using SqliteWriteStatementMockBase::SqliteWriteStatementMockBase;
};
