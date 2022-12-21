// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitereadwritestatementmock.h"

#include "sqlitedatabasemock.h"

SqliteReadWriteStatementMockBase::SqliteReadWriteStatementMockBase(Utils::SmallStringView sqlStatement,
                                                                   SqliteDatabaseMock &databaseMock)
    : sqlStatement{sqlStatement}
{
    databaseMock.prepare(sqlStatement);
}
