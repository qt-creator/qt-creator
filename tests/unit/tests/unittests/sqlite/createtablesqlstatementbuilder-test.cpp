// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

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

TEST_F(CreateTableSqlStatementBuilder, is_not_valid_after_creation)
{
    ASSERT_FALSE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, is_valid_after_binding)
{
    bindValues();

    ASSERT_TRUE(builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, invalid_after_clear)
{
    bindValues();

    builder.clear();

    ASSERT_TRUE(!builder.isValid());
}

TEST_F(CreateTableSqlStatementBuilder, no_sql_statement_after_clear)
{
    bindValues();
    builder.sqlStatement();

    builder.clear();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, sql_statement)
{
    bindValues();

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, add_column_to_existing_columns)
{
    bindValues();

    builder.addColumn("number2", ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC, number2 REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, change_table)
{
    bindValues();

    builder.setTableName("test2");

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test2(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, is_invalid_after_clear_colums_only)
{
    bindValues();
    builder.sqlStatement();

    builder.clearColumns();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateTableSqlStatementBuilder, clear_columns_and_add_column_new_columns)
{
    bindValues();
    builder.clearColumns();

    builder.addColumn("name3", ColumnType::Text);
    builder.addColumn("number3", ColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(name3 TEXT, number3 REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, set_witout_row_id)
{
    bindValues();

    builder.setUseWithoutRowId(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC) WITHOUT ROWID");
}

TEST_F(CreateTableSqlStatementBuilder, set_column_definitions)
{
    builder.setTableName("test");

    builder.setColumns(createColumns());

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, unique_contraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Unique{}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER UNIQUE)");
}

TEST_F(CreateTableSqlStatementBuilder, if_not_exits_modifier)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, {});

    builder.setUseIfNotExists(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE IF NOT EXISTS test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, temporary_table)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer, {});

    builder.setUseTemporaryTable(true);

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TEMPORARY TABLE test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_without_column)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", ""}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER REFERENCES otherTable)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_with_column)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_update_no_action)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_update_restrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Restrict}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_update_set_null)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetNull}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_update_set_default)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetDefault}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_update_cascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Cascade}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_no_action)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn))");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_restrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Restrict}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE RESTRICT)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_set_null)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetNull}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_set_default)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetDefault}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET DEFAULT)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_cascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Cascade}});

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE CASCADE)");
}

TEST_F(CreateTableSqlStatementBuilder, foreign_key_delete_and_update_action)
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

TEST_F(CreateTableSqlStatementBuilder, foreign_key_deferred)
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

TEST_F(CreateTableSqlStatementBuilder, not_null_constraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER NOT NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, not_null_and_unique_constraint)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::Unique{}, Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE NOT NULL)");
}

TEST_F(CreateTableSqlStatementBuilder, check)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::Check{"id != ''"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT CHECK (id != ''))");
}

TEST_F(CreateTableSqlStatementBuilder, default_value_int)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::DefaultValue{1LL}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER DEFAULT 1)");
}

TEST_F(CreateTableSqlStatementBuilder, default_value_float)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Real, {Sqlite::DefaultValue{1.1}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id REAL DEFAULT 1.100000)");
}

TEST_F(CreateTableSqlStatementBuilder, default_value_string)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::DefaultValue{"foo"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT DEFAULT 'foo')");
}

TEST_F(CreateTableSqlStatementBuilder, default_expression)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      ColumnType::Integer,
                      {Sqlite::DefaultExpression{"SELECT name FROM foo WHERE id=?"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER DEFAULT (SELECT name FROM foo WHERE id=?))");
}

TEST_F(CreateTableSqlStatementBuilder, collation)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Text, {Sqlite::Collate{"unicode"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT COLLATE unicode)");
}

TEST_F(CreateTableSqlStatementBuilder, generated_always_stored)
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

TEST_F(CreateTableSqlStatementBuilder, generated_always_virtual)
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

TEST_F(CreateTableSqlStatementBuilder, primary_key_autoincrement)
{
    builder.setTableName("test");

    builder.addColumn("id", ColumnType::Integer, {Sqlite::PrimaryKey{Sqlite::AutoIncrement::Yes}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT)");
}

TEST_F(CreateTableSqlStatementBuilder, blob_type)
{
    builder.setTableName("test");

    builder.addColumn("data", ColumnType::Blob);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(data BLOB)");
}

TEST_F(CreateTableSqlStatementBuilder, table_primary_key_constaint)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer);
    builder.addColumn("text", ColumnType::Text);

    builder.addConstraint(Sqlite::TablePrimaryKey{{"id, text"}});
    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER, text TEXT, PRIMARY KEY(id, text))");
}

TEST_F(CreateTableSqlStatementBuilder, none_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::None);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id)");
}

TEST_F(CreateTableSqlStatementBuilder, numeric_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Numeric);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id NUMERIC)");
}

TEST_F(CreateTableSqlStatementBuilder, integer_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Integer);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER)");
}

TEST_F(CreateTableSqlStatementBuilder, real_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Real);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id REAL)");
}

TEST_F(CreateTableSqlStatementBuilder, text_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", ColumnType::Text);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id TEXT)");
}

TEST_F(CreateTableSqlStatementBuilder, blob_column_type_string_conversion)
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

TEST_F(CreateStrictTableSqlStatementBuilder, is_not_valid_after_creation)
{
    ASSERT_FALSE(builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, is_valid_after_binding)
{
    bindValues();

    ASSERT_TRUE(builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, invalid_after_clear)
{
    bindValues();

    builder.clear();

    ASSERT_TRUE(!builder.isValid());
}

TEST_F(CreateStrictTableSqlStatementBuilder, no_sql_statement_after_clear)
{
    bindValues();
    builder.sqlStatement();

    builder.clear();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateStrictTableSqlStatementBuilder, sql_statement)
{
    bindValues();

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, add_column_to_existing_columns)
{
    bindValues();

    builder.addColumn("number2", StrictColumnType::Real);

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY, number2 REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, change_table)
{
    bindValues();

    builder.setTableName("test2");

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test2(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, is_invalid_after_clear_colums_only)
{
    bindValues();
    builder.sqlStatement();

    builder.clearColumns();

    ASSERT_THROW(builder.sqlStatement(), SqlStatementBuilderException);
}

TEST_F(CreateStrictTableSqlStatementBuilder, clear_columns_and_add_column_new_columns)
{
    bindValues();
    builder.clearColumns();

    builder.addColumn("name3", StrictColumnType::Text);
    builder.addColumn("number3", StrictColumnType::Real);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(name3 TEXT, number3 REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, set_witout_row_id)
{
    bindValues();

    builder.setUseWithoutRowId(true);

    ASSERT_THAT(
        builder.sqlStatement(),
        "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) WITHOUT ROWID, STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, set_column_definitions)
{
    builder.setTableName("test");

    builder.setColumns(createColumns());

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT, number ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, unique_contraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Unique{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, if_not_exits_modifier)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer, {});

    builder.setUseIfNotExists(true);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE IF NOT EXISTS test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, temporary_table)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer, {});

    builder.setUseTemporaryTable(true);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TEMPORARY TABLE test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_without_column)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", ""}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER REFERENCES otherTable) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_with_column)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_update_no_action)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_update_restrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Restrict}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE "
                "RESTRICT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_update_set_null)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetNull}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_update_set_default)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::SetDefault}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE SET "
                "DEFAULT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_update_cascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON UPDATE "
                "CASCADE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_no_action)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {ForeignKey{"otherTable", "otherColumn"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_restrict)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Restrict}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE "
                "RESTRICT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_set_null)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetNull}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET "
                "NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_set_default)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::SetDefault}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE SET "
                "DEFAULT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_cascade)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {ForeignKey{"otherTable", "otherColumn", {}, ForeignKeyAction::Cascade}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER REFERENCES otherTable(otherColumn) ON DELETE "
                "CASCADE) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_delete_and_update_action)
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

TEST_F(CreateStrictTableSqlStatementBuilder, foreign_key_deferred)
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

TEST_F(CreateStrictTableSqlStatementBuilder, not_null_constraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER NOT NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, not_null_and_unique_constraint)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::Unique{}, Sqlite::NotNull{}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER UNIQUE NOT NULL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, check)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::Check{"id != ''"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT CHECK (id != '')) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, default_value_int)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Integer, {Sqlite::DefaultValue{1LL}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id INTEGER DEFAULT 1) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, default_value_float)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Real, {Sqlite::DefaultValue{1.1}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id REAL DEFAULT 1.100000) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, default_value_string)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::DefaultValue{"foo"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT DEFAULT 'foo') STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, default_expression)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {Sqlite::DefaultExpression{"SELECT name FROM foo WHERE id=?"}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER DEFAULT (SELECT name FROM foo WHERE id=?)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, collation)
{
    builder.setTableName("test");

    builder.addColumn("id", StrictColumnType::Text, {Sqlite::Collate{"unicode"}});

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(id TEXT COLLATE unicode) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, generated_always_stored)
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

TEST_F(CreateStrictTableSqlStatementBuilder, generated_always_virtual)
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

TEST_F(CreateStrictTableSqlStatementBuilder, primary_key_autoincrement)
{
    builder.setTableName("test");

    builder.addColumn("id",
                      StrictColumnType::Integer,
                      {Sqlite::PrimaryKey{Sqlite::AutoIncrement::Yes}});

    ASSERT_THAT(builder.sqlStatement(),
                "CREATE TABLE test(id INTEGER PRIMARY KEY AUTOINCREMENT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, blob_type)
{
    builder.setTableName("test");

    builder.addColumn("data", StrictColumnType::Blob);

    ASSERT_THAT(builder.sqlStatement(), "CREATE TABLE test(data BLOB) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, table_primary_key_constaint)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer);
    builder.addColumn("text", StrictColumnType::Text);

    builder.addConstraint(Sqlite::TablePrimaryKey{{"id, text"}});
    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER, text TEXT, PRIMARY KEY(id, text)) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, any_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Any);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id ANY) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, int_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Int);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, integer_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Integer);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id INTEGER) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, real_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Real);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id REAL) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, text_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Text);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id TEXT) STRICT");
}

TEST_F(CreateStrictTableSqlStatementBuilder, blob_column_type_string_conversion)
{
    builder.setTableName("test");
    builder.addColumn("id", StrictColumnType::Blob);

    auto statement = builder.sqlStatement();

    ASSERT_THAT(statement, "CREATE TABLE test(id BLOB) STRICT");
}

} // namespace
