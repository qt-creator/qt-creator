// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <sqliteindex.h>

namespace {

using Sqlite::Exception;
using Sqlite::Index;
using Sqlite::IndexType;

TEST(Index, OneColumn)
{
    Index index{"tableName", {"column1"}};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement, Eq("CREATE INDEX IF NOT EXISTS index_tableName_column1 ON tableName(column1)"));
}

TEST(Index, TwoColumn)
{
    Index index{"tableName", {"column1", "column2"}};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement, Eq("CREATE INDEX IF NOT EXISTS index_tableName_column1_column2 ON tableName(column1, column2)"));
}

TEST(Index, EmptyTableName)
{
    Index index{"", {"column1", "column2"}};

    ASSERT_THROW(index.sqlStatement(), Exception);
}

TEST(Index, EmptyColumns)
{
    Index index{"tableName", {}};

    ASSERT_THROW(index.sqlStatement(), Exception);
}

TEST(Index, UniqueIndex)
{
    Index index{"tableName", {"column1"}, IndexType::Unique};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement, Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_tableName_column1 ON tableName(column1)"));
}

TEST(Index, Condition)
{
    Index index{"tableName", {"column1"}, IndexType::Normal, "column1 IS NOT NULL"};

    auto sqlStatement = index.sqlStatement();

    ASSERT_THAT(sqlStatement,
                Eq("CREATE INDEX IF NOT EXISTS index_tableName_column1 ON tableName(column1) WHERE "
                   "column1 IS NOT NULL"));
}
}
