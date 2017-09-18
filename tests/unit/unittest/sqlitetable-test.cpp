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
#include "spydummy.h"

#include <sqlitecolumn.h>
#include <mocksqlitedatabase.h>
#include <sqlitetable.h>

namespace {

using Sqlite::ColumnType;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::Column;
using Sqlite::Database;

class SqliteTable : public ::testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Sqlite::Table table;
    Utils::SmallString tableName = "testTable";
};


TEST_F(SqliteTable, ColumnIsAddedToTable)
{
    table.setUseWithoutRowId(true);

    ASSERT_TRUE(table.useWithoutRowId());
}

TEST_F(SqliteTable, SetTableName)
{
    table.setName(tableName.clone());

    ASSERT_THAT(table.name(), tableName);
}

TEST_F(SqliteTable, SetUseWithoutRowid)
{
    table.setUseWithoutRowId(true);

    ASSERT_TRUE(table.useWithoutRowId());
}

TEST_F(SqliteTable, AddIndex)
{
    table.setName(tableName.clone());
    auto &column = table.addColumn("name");
    auto &column2 = table.addColumn("value");

    auto index = table.addIndex({column, column2});

    ASSERT_THAT(Utils::SmallStringView(index.sqlStatement()),
                Eq("CREATE INDEX IF NOT EXISTS index_testTable_name_value ON testTable(name, value)"));
}

TEST_F(SqliteTable, InitializeTable)
{
    table.setName(tableName.clone());
    table.setUseIfNotExists(true);
    table.setUseTemporaryTable(true);
    table.setUseWithoutRowId(true);
    table.addColumn("name");
    table.addColumn("value");

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE IF NOT EXISTS testTable(name NUMERIC, value NUMERIC) WITHOUT ROWID")));

    table.initialize(mockDatabase);
}

TEST_F(SqliteTable, InitializeTableWithIndex)
{
    InSequence sequence;
    table.setName(tableName.clone());
    auto &column = table.addColumn("name");
    auto &column2 = table.addColumn("value");
    table.addIndex({column});
    table.addIndex({column2});

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE testTable(name NUMERIC, value NUMERIC)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_testTable_name ON testTable(name)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_testTable_value ON testTable(value)")));

    table.initialize(mockDatabase);
}

}
