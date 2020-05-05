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

#include <createtablesqlstatementbuilder.h>
#include <sqlstatementbuilderexception.h>

namespace {

using Sqlite::Column;
using Sqlite::ColumnType;
using Sqlite::Contraint;
using Sqlite::Enforment;
using Sqlite::ForeignKeyAction;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::SqliteColumns;
using Sqlite::SqlStatementBuilderException;

class CreateTableSqlStatementBuilder : public ::testing::Test
{
protected:
    void bindValues();
    static SqliteColumns createColumns();

protected:
    Sqlite::CreateTableSqlStatementBuilder builder;
};

TEST_F(CreateTableSqlStatementBuilder, IsNotValidAfterCreation)
{
    ASSERT_FALSE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, IsValidAfterBinding)
{
    bindValues();

    ASSERT_TRUE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, InvalidAfterClear)
{
    bindValues();

    builder.clear();

    ASSERT_TRUE(!builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, NoSqlStatementAfterClear)
{
    bindValues();
    builder.sqlStatement();

    builder.clear();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, SqlStatement)
{
    bindValues();

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, AddColumnToExistingColumns)
{
    bindValues();

    builder.addColumn("number2", ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC, number2 REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, ChangeTable)
{
    bindValues();

    builder.setTableName("test2");

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test2(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)"
                );
}

TEST_F(CreateTableSqlStatementBuilder, IsInvalidAfterClearColumsOnly)
{
    bindValues();
    builder.sqlStatement();

    builder.clearColumns();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, ClearColumnsAndAddColumnNewColumns)
{
    bindValues();
    builder.clearColumns();

    builder.addColumn("name3", ColumnType::Text);
    builder.addColumn("number3", ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(name3 TEXT, number3 REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, SetWitoutRowId)
{
    bindValues();

    builder.setUseWithoutRowId(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC) WITHOUT ROWID");
}

TEST_F(CreateTableSqlStatementBuilder, SetColumnDefinitions)
{
    builder.clear();
    builder.setTableName("test");

    builder.setColumns(createColumns());

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, UniqueContraint)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, Contraint::Unique);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER UNIQUE)");
}

TEST_F(CreateTableSqlStatementBuilder, IfNotExitsModifier)
{
    builder.clear();
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, Contraint::NoConstraint);

    builder.setUseIfNotExists(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE IF NOT EXISTS test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, TemporaryTable)
{
    builder.clear();
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, Contraint::NoConstraint);

    builder.setUseTemporaryTable(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TEMPORARY TABLE test(id INTEGER)");
}

void CreateTableSqlStatementBuilder::bindValues()
{
    builder.clear();
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, Contraint::PrimaryKey);
    builder.addColumn("name", ColumnType::Text);
    builder.addColumn("number",ColumnType:: Numeric);
}

SqliteColumns CreateTableSqlStatementBuilder::createColumns()
{
    SqliteColumns columns;
    columns.emplace_back("", "id", ColumnType::Integer, Contraint::PrimaryKey);
    columns.emplace_back("", "name", ColumnType::Text);
    columns.emplace_back("", "number", ColumnType::Numeric);

    return columns;
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyWithoutColumn)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, Contraint::ForeignKey, {"otherTable", ""});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER REFERENCES otherTable)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyWithColumn)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, Contraint::ForeignKey, {"otherTable", "otherColumn"});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateNoAction)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, Contraint::ForeignKey, {"otherTable", "otherColumn"});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateRestrict)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", ForeignKeyAction::Restrict});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateSetNull)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", ForeignKeyAction::SetNull});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateSetDefault)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", ForeignKeyAction::SetDefault});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateCascade)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", ForeignKeyAction::Cascade});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteNoAction)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, Contraint::ForeignKey, {"otherTable", "otherColumn"});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteRestrict)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", {}, ForeignKeyAction::Restrict});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteSetNull)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", {}, ForeignKeyAction::SetNull});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteSetDefault)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", {}, ForeignKeyAction::SetDefault});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteCascade)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable", "otherColumn", {}, ForeignKeyAction::Cascade});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteAndUpdateAction)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable",
                       "otherColumn",
                       ForeignKeyAction::SetDefault,
                       ForeignKeyAction::Cascade});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeferred)
{
    builder.clear();
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      Contraint::ForeignKey,
                      {"otherTable",
                       "otherColumn",
                       ForeignKeyAction::SetDefault,
                       ForeignKeyAction::Cascade,
                       Enforment::Deferred});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED)");
}
} // namespace
