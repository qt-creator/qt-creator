/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "googletest.h"

#include <sqlstatementbuilderexception.h>
#include <sqlstatementbuilder.h>

using namespace ::testing;

using Sqlite::ColumnType;
using Sqlite::SqlStatementBuilder;
using Sqlite::SqlStatementBuilderException;

using SV = Utils::SmallStringVector;

TEST(SqlStatementBuilder, Bind)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table WHERE $column = 'foo' AND rowid=$row AND rowid IN ($rows)");

    sqlStatementBuilder.bind("$columns", SV{"name", "number"});
    sqlStatementBuilder.bind("$column", "name");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bind("$row", 20);
    sqlStatementBuilder.bind("$rows", {1, 2, 3});

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test WHERE name = 'foo' AND rowid=20 AND rowid IN (1, 2, 3)");
}

TEST(SqlStatementBuilder, BindEmpty)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table$emptyPart");
    sqlStatementBuilder.bind("$columns", SV{"name", "number"});
    sqlStatementBuilder.bind("$table", "test");

    sqlStatementBuilder.bindEmptyText("$emptyPart");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");
}

TEST(SqlStatementBuilder, BindFailure)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    Utils::SmallStringVector columns;

    ASSERT_THROW(sqlStatementBuilder.bind("$columns", ""), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind("columns", "test"), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind("$columns", columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithInsertTemplateParameters("$columns", columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithUpdateTemplateParameters("$columns", columns), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, BindWithInsertTemplateParameters)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("INSERT OR IGNORE INTO $table ($columns) VALUES ($values)");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bind("$columns", columns);
    sqlStatementBuilder.bindWithInsertTemplateParameters("$values", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "INSERT OR IGNORE INTO test (name, number) VALUES (?, ?)");
}

TEST(SqlStatementBuilder, BindWithUpdateTemplateParameters)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("UPDATE $table SET $columnValues WHERE id=?");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bindWithUpdateTemplateParameters("$columnValues", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "UPDATE test SET name=?, number=? WHERE id=?");
}

TEST(SqlStatementBuilder, BindWithUpdateTemplateNames)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("UPDATE $table SET $columnValues WHERE id=@id");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bindWithUpdateTemplateNames("$columnValues", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "UPDATE test SET name=@name, number=@number WHERE id=@id");
}

TEST(SqlStatementBuilder, ClearOnRebinding)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");
    sqlStatementBuilder.bind("$table", "test");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");

    sqlStatementBuilder.bind("$table", "test2");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test2");
}

TEST(SqlStatementBuilder, ClearBinding)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");
    sqlStatementBuilder.bind("$table", "test");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");

    sqlStatementBuilder.clear();

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, ColumnType)
{
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Numeric), "NUMERIC");
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Integer), "INTEGER");
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Real), "REAL");
    ASSERT_THAT(SqlStatementBuilder::columnTypeToString(ColumnType::Text), "TEXT");
    ASSERT_TRUE(SqlStatementBuilder::columnTypeToString(ColumnType::None).isEmpty());
}

TEST(SqlStatementBuilder, SqlStatementFailure)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, IsBuild)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");
    sqlStatementBuilder.bind("$table", "test");

    ASSERT_FALSE(sqlStatementBuilder.isBuild());

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");

    ASSERT_TRUE(sqlStatementBuilder.isBuild());

    sqlStatementBuilder.clear();

    ASSERT_FALSE(sqlStatementBuilder.isBuild());
}
