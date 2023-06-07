// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlstatementbuilderexception.h>
#include <sqlstatementbuilder.h>

using namespace ::testing;

using Sqlite::ColumnType;
using Sqlite::SqlStatementBuilder;
using Sqlite::SqlStatementBuilderException;

using SV = Utils::SmallStringVector;

TEST(SqlStatementBuilder, bind)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table WHERE $column = 'foo' AND rowid=$row AND rowid IN ($rows)");

    sqlStatementBuilder.bind("$columns", SV{"name", "number"});
    sqlStatementBuilder.bind("$column", "name");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bind("$row", 20);
    sqlStatementBuilder.bind("$rows", {1, 2, 3});

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test WHERE name = 'foo' AND rowid=20 AND rowid IN (1, 2, 3)");
}

TEST(SqlStatementBuilder, bind_empty)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table$emptyPart");
    sqlStatementBuilder.bind("$columns", SV{"name", "number"});
    sqlStatementBuilder.bind("$table", "test");

    sqlStatementBuilder.bindEmptyText("$emptyPart");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");
}

TEST(SqlStatementBuilder, bind_failure)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    Utils::SmallStringVector columns;

    ASSERT_THROW(sqlStatementBuilder.bind("$columns", ""), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind("columns", "test"), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bind("$columns", columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithInsertTemplateParameters("$columns", columns), SqlStatementBuilderException);
    ASSERT_THROW(sqlStatementBuilder.bindWithUpdateTemplateParameters("$columns", columns), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, bind_with_insert_template_parameters)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("INSERT OR IGNORE INTO $table ($columns) VALUES ($values)");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bind("$columns", columns);
    sqlStatementBuilder.bindWithInsertTemplateParameters("$values", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "INSERT OR IGNORE INTO test (name, number) VALUES (?, ?)");
}

TEST(SqlStatementBuilder, bind_with_update_template_parameters)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("UPDATE $table SET $columnValues WHERE id=?");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bindWithUpdateTemplateParameters("$columnValues", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "UPDATE test SET name=?, number=? WHERE id=?");
}

TEST(SqlStatementBuilder, bind_with_update_template_names)
{
    Utils::SmallStringVector columns = {"name", "number"};

    SqlStatementBuilder sqlStatementBuilder("UPDATE $table SET $columnValues WHERE id=@id");
    sqlStatementBuilder.bind("$table", "test");
    sqlStatementBuilder.bindWithUpdateTemplateNames("$columnValues", columns);

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "UPDATE test SET name=@name, number=@number WHERE id=@id");
}

TEST(SqlStatementBuilder, clear_on_rebinding)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");
    sqlStatementBuilder.bind("$table", "test");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");

    sqlStatementBuilder.bind("$table", "test2");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test2");
}

TEST(SqlStatementBuilder, clear_binding)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");
    sqlStatementBuilder.bind("$table", "test");

    ASSERT_THAT(sqlStatementBuilder.sqlStatement(), "SELECT name, number FROM test");

    sqlStatementBuilder.clear();

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, sql_statement_failure)
{
    SqlStatementBuilder sqlStatementBuilder("SELECT $columns FROM $table");

    sqlStatementBuilder.bind("$columns", "name, number");

    ASSERT_THROW(sqlStatementBuilder.sqlStatement(), SqlStatementBuilderException);
}

TEST(SqlStatementBuilder, is_build)
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
