// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sqlitebasestatement.h>
template<int ResultCount, int BindParameterCount = 0>
class SqliteTestStatement
    : public Sqlite::StatementImplementation<Sqlite::BaseStatement, ResultCount, BindParameterCount>
{
    using Base = Sqlite::StatementImplementation<Sqlite::BaseStatement, ResultCount, BindParameterCount>;

public:
    explicit SqliteTestStatement(Utils::SmallStringView sqlStatement, Sqlite::Database &database)
        : Base(sqlStatement, database)
    {}
};

