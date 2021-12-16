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
using Sqlite::ConstraintType;
using Sqlite::Enforment;
using Sqlite::ForeignKey;
using Sqlite::ForeignKeyAction;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::PrimaryKey;
using Sqlite::SqlStatementBuilderException;
using Sqlite::StrictColumnType;
using Sqlite::Unique;

class CreateTableSqlStatementBuilder : public ::testing::Test
{
protected:
    using Columns = ::Sqlite::Columns;
    using Builder = Sqlite::CreateTableSqlStatementBuilder<ColumnType>;
    void bindValues()
    {
        builder.clear();
        builder.setTableName("test");
        builder.addColumn("id", ColumnType::Integer, {PrimaryKey{}});
        builder.addColumn("name", ColumnType::Text);
        builder.addColumn("number", ColumnType::Numeric);
    }

    static Columns createColumns()
    {
        Columns columns;
        columns.emplace_back("", "id", ColumnType::Integer, Sqlite::Constraints{PrimaryKey{}});
        columns.emplace_back("", "name", ColumnType::Text);
        columns.emplace_back("", "number", ColumnType::Numeric);

        return columns;
    }

protected:
    Builder builder;
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
                "CREATE TABLE test2(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
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
    builder.setTableName("test");

    builder.setColumns(createColumns());

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, UniqueContraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Unique{}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER UNIQUE)");
}

TEST_F(CreateTableSqlStatementBuilder, IfNotExitsModifier)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, {});

    builder.setUseIfNotExists(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE IF NOT EXISTS test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, TemporaryTable)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, {});

    builder.setUseTemporaryTable(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TEMPORARY TABLE test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyWithoutColumn)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", ""}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER REFERENCES otherTable)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyWithColumn)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateNoAction)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateRestrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Restrict}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateSetNull)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetNull}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateSetDefault)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetDefault}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyUpdateCascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Cascade}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteNoAction)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteRestrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Restrict}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteSetNull)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetNull}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteSetDefault)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetDefault}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteCascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Cascade}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeleteAndUpdateAction)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable",
                                  "otherColumn",
                                  ForeignKeyAction::SetDefault,
                                  ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, ForeignKeyDeferred)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable",
                                  "otherColumn",
                                  ForeignKeyAction::SetDefault,
                                  ForeignKeyAction::Cascade,
                                  Enforment::Deferred}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED)");
}

TEST_F(CreateTableSqlStatementBuilder, NotNullConstraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER NOT NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, NotNullAndUniqueConstraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::Unique{}, Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE NOT NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, Check)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::Check{"id != ''"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT CHECK (id != ''))");
}

TEST_F(CreateTableSqlStatementBuilder, DefaultValueInt)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::DefaultValue{1LL}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER DEFAULT 1)");
}

TEST_F(CreateTableSqlStatementBuilder, DefaultValueFloat)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Real, {Sqlite::DefaultValue{1.1}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id REAL DEFAULT 1.100000)");
}

TEST_F(CreateTableSqlStatementBuilder, DefaultValueString)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::DefaultValue{"foo"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT DEFAULT 'foo')");
}

TEST_F(CreateTableSqlStatementBuilder, DefaultExpression)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {Sqlite::DefaultExpression{"SELECT name FROM foo WHERE id=?"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER DEFAULT (SELECT name FROM foo WHERE id=?))");
}

TEST_F(CreateTableSqlStatementBuilder, Collation)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::Collate{"unicode"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT COLLATE unicode)");
}

TEST_F(CreateTableSqlStatementBuilder, GeneratedAlwaysStored)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Text,
                      {Sqlite::GeneratedAlways{"SELECT name FROM foo WHERE id=?",
                                               Sqlite::GeneratedAlwaysStorage::Stored}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id TEXT GENERATED ALWAYS AS (SELECT name FROM foo WHERE id=?) STORED)");
}

TEST_F(CreateTableSqlStatementBuilder, GeneratedAlwaysVirtual)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Text,
                      {Sqlite::GeneratedAlways{"SELECT name FROM foo WHERE id=?",
                                               Sqlite::GeneratedAlwaysStorage::Virtual}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id TEXT GENERATED ALWAYS AS (SELECT name FROM foo WHERE id=?) "
                "VIRTUAL)");
}

TEST_F(CreateTableSqlStatementBuilder, PrimaryKeyAutoincrement)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::PrimaryKey{Sqlite::AutoIncrement::Yes}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT)");
}

TEST_F(CreateTableSqlStatementBuilder, BlobType)
{
    builder.setTableName("test");

    builder.addColumn("data", ColumnType::Blob);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(data BLOB)");
}

TEST_F(CreateTableSqlStatementBuilder, TablePrimaryKeyConstaint)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer);
    builder.addColumn("text", ColumnType::Text);

    builder.addConstraint(Sqlite::TablePrimaryKey{{"id, text"}});
    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER, text TEXT, PRIMARY KEY(id, text))");
}

TEST_F(CreateTableSqlStatementBuilder, NoneColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::None);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id)");
}

TEST_F(CreateTableSqlStatementBuilder, NumericColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Numeric);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, IntegerColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, RealColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Real);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, TextColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Text);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id TEXT)");
}

TEST_F(CreateTableSqlStatementBuilder, BlobColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Blob);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id BLOB)");
}

class CreateStrictTableSqlStatementBuilder : public ::testing::Test
{
protected:
    using Columns = ::Sqlite::StrictColumns;

    void bindValues()
    {
        builder.clear();
        builder.setTableName("test");
        builder.addColumn("id", StrictColumnType::Integer, {PrimaryKey{}});
        builder.addColumn("name", StrictColumnType::Text);
        builder.addColumn("number", StrictColumnType::Any);
    }

    static Columns createColumns()
    {
        Columns columns;
        columns.emplace_back("", "id", StrictColumnType::Integer, Sqlite::Constraints{PrimaryKey{}});
        columns.emplace_back("", "name", StrictColumnType::Text);
        columns.emplace_back("", "number", StrictColumnType::Any);

        return columns;
    }

protected:
    Sqlite::CreateTableSqlStatementBuilder<StrictColumnType> builder;
};

TEST_F(CreateStrictTableSqlStatementBuilder, IsNotValidAfterCreation)
{
    ASSERT_FALSE(builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, IsValidAfterBinding)
{
    bindValues();

    ASSERT_TRUE(builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, InvalidAfterClear)
{
    bindValues();

    builder.clear();

    ASSERT_TRUE(!builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, NoSqlStatementAfterClear)
{
    bindValues();
    builder.sqlStatement();

    builder.clear();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateStrictTableSqlStatementBuilder, SqlStatement)
{
    bindValues();

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, AddColumnToExistingColumns)
{
    bindValues();

    builder.addColumn("number2", StrictColumnType::Real);

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY, number2 REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ChangeTable)
{
    bindValues();

    builder.setTableName("test2");

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test2(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, IsInvalidAfterClearColumsOnly)
{
    bindValues();
    builder.sqlStatement();

    builder.clearColumns();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateStrictTableSqlStatementBuilder, ClearColumnsAndAddColumnNewColumns)
{
    bindValues();
    builder.clearColumns();

    builder.addColumn("name3", StrictColumnType::Text);
    builder.addColumn("number3", StrictColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(name3 TEXT, number3 REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, SetWitoutRowId)
{
    bindValues();

    builder.setUseWithoutRowId(true);

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) WITHOUT ROWID STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, SetColumnDefinitions)
{
    builder.setTableName("test");

    builder.setColumns(createColumns());

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, UniqueContraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Unique{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, IfNotExitsModifier)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer, {});

    builder.setUseIfNotExists(true);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE IF NOT EXISTS test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, TemporaryTable)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer, {});

    builder.setUseTemporaryTable(true);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TEMPORARY TABLE test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyWithoutColumn)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", ""}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER REFERENCES otherTable) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyWithColumn)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyUpdateNoAction)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyUpdateRestrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Restrict}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE "
                "RESTRICT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyUpdateSetNull)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetNull}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyUpdateSetDefault)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetDefault}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyUpdateCascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE "
                "CASCADE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteNoAction)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteRestrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Restrict}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE "
                "RESTRICT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteSetNull)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetNull}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET "
                "NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteSetDefault)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetDefault}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET "
                "DEFAULT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteCascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE "
                "CASCADE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeleteAndUpdateAction)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable",
                                  "otherColumn",
                                  ForeignKeyAction::SetDefault,
                                  ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, ForeignKeyDeferred)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable",
                                  "otherColumn",
                                  ForeignKeyAction::SetDefault,
                                  ForeignKeyAction::Cascade,
                                  Enforment::Deferred}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT ON DELETE CASCADE DEFERRABLE INITIALLY DEFERRED) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, NotNullConstraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER NOT NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, NotNullAndUniqueConstraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::Unique{}, Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE NOT NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, Check)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::Check{"id != ''"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT CHECK (id != '')) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, DefaultValueInt)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::DefaultValue{1LL}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER DEFAULT 1) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, DefaultValueFloat)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Real, {Sqlite::DefaultValue{1.1}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id REAL DEFAULT 1.100000) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, DefaultValueString)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::DefaultValue{"foo"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT DEFAULT 'foo') STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, DefaultExpression)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {Sqlite::DefaultExpression{"SELECT name FROM foo WHERE id=?"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER DEFAULT (SELECT name FROM foo WHERE id=?)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, Collation)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::Collate{"unicode"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT COLLATE unicode) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, GeneratedAlwaysStored)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Text,
                      {Sqlite::GeneratedAlways{"SELECT name FROM foo WHERE id=?",
                                               Sqlite::GeneratedAlwaysStorage::Stored}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id TEXT GENERATED ALWAYS AS (SELECT name FROM foo WHERE id=?) "
                "STORED) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, GeneratedAlwaysVirtual)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Text,
                      {Sqlite::GeneratedAlways{"SELECT name FROM foo WHERE id=?",
                                               Sqlite::GeneratedAlwaysStorage::Virtual}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id TEXT GENERATED ALWAYS AS (SELECT name FROM foo WHERE id=?) "
                "VIRTUAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, PrimaryKeyAutoincrement)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {Sqlite::PrimaryKey{Sqlite::AutoIncrement::Yes}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, BlobType)
{
    builder.setTableName("test");

    builder.addColumn("data", StrictColumnType::Blob);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(data BLOB) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, TablePrimaryKeyConstaint)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer);
    builder.addColumn("text", StrictColumnType::Text);

    builder.addConstraint(Sqlite::TablePrimaryKey{{"id, text"}});
    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER, text TEXT, PRIMARY KEY(id, text)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, AnyColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Any);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, IntColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Int);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, IntegerColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, RealColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Real);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, TextColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Text);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id TEXT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, BlobColumnTypeStringConversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Blob);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id BLOB) STRICT");
}

} // namespace
