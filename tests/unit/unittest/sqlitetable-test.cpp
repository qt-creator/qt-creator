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
#include <sqlitedatabase.h>
#include <sqlitetable.h>

namespace {

using Sqlite::SqliteColumn;
using Sqlite::SqliteDatabase;

class SqliteTable : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    SqliteColumn *addColumn(Utils::SmallString columnName);

    SpyDummy spyDummy;
    SqliteDatabase database;
    Sqlite::SqliteTable *table = new Sqlite::SqliteTable;
    Utils::SmallString tableName = "testTable";
};


TEST_F(SqliteTable, ColumnIsAddedToTable)
{
    table->setUseWithoutRowId(true);

    ASSERT_TRUE(table->useWithoutRowId());
}

TEST_F(SqliteTable, SetTableName)
{
    table->setName(tableName.clone());

    ASSERT_THAT(table->name(), tableName);
}

TEST_F(SqliteTable, SetUseWithoutRowid)
{
    table->setUseWithoutRowId(true);

    ASSERT_TRUE(table->useWithoutRowId());
}

TEST_F(SqliteTable, TableIsReadyAfterOpenDatabase)
{
    table->setName(tableName.clone());
    addColumn("name");

    database.open();

    ASSERT_TRUE(table->isReady());
}

void SqliteTable::SetUp()
{
    database.setJournalMode(JournalMode::Memory);
    database.setDatabaseFilePath( QStringLiteral(":memory:"));
    database.addTable(table);
}

void SqliteTable::TearDown()
{
    if (database.isOpen())
        database.close();
    table = nullptr;
}

SqliteColumn *SqliteTable::addColumn(Utils::SmallString columnName)
{
    SqliteColumn *newSqliteColum = new SqliteColumn;

    newSqliteColum->setName(std::move(columnName));

    table->addColumn(newSqliteColum);

    return newSqliteColum;
}
}
