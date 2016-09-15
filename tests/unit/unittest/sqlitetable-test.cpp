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
#include <utf8string.h>

#include <QSignalSpy>
#include <QVariant>

namespace {

class SqliteTable : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    SqliteColumn *addColumn(const Utf8String &columnName);

    SpyDummy spyDummy;
    SqliteDatabase *database = nullptr;
    ::SqliteTable *table = nullptr;
    Utf8String tableName = Utf8StringLiteral("testTable");
};


TEST_F(SqliteTable, ColumnIsAddedToTable)
{
    table->setUseWithoutRowId(true);

    ASSERT_TRUE(table->useWithoutRowId());
}

TEST_F(SqliteTable, SetTableName)
{
    table->setName(tableName);

    ASSERT_THAT(table->name(), tableName);
}

TEST_F(SqliteTable, SetUseWithoutRowid)
{
    table->setUseWithoutRowId(true);

    ASSERT_TRUE(table->useWithoutRowId());
}

TEST_F(SqliteTable, TableIsReadyAfterOpenDatabase)
{
    QSignalSpy signalSpy(&spyDummy, &SpyDummy::tableIsReady);
    table->setName(tableName);
    addColumn(Utf8StringLiteral("name"));

    database->open();

    ASSERT_TRUE(signalSpy.wait(100000));
}

void SqliteTable::SetUp()
{
    table = new ::SqliteTable;
    QObject::connect(table, &::SqliteTable::tableIsReady, &spyDummy, &SpyDummy::tableIsReady);

    database = new SqliteDatabase;
    database->setJournalMode(JournalMode::Memory);
    database->setDatabaseFilePath( QStringLiteral(":memory:"));
    database->addTable(table);
}

void SqliteTable::TearDown()
{
    database->close();
    delete database;
    database = nullptr;
    table = nullptr;
}

SqliteColumn *SqliteTable::addColumn(const Utf8String &columnName)
{
    SqliteColumn *newSqliteColum = new SqliteColumn;

    newSqliteColum->setName(columnName);

    table->addColumn(newSqliteColum);

    return newSqliteColum;
}
}
