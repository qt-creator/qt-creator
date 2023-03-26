// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitewritestatementmock.h"

#include "sqlitedatabasemock.h"

SqliteWriteStatementMockBase::SqliteWriteStatementMockBase(Utils::SmallStringView sqlStatement,
                                                           SqliteDatabaseMock &database)
    : sqlStatement(sqlStatement)
{
    database.prepare(sqlStatement);
}
