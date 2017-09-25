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

#include <sqlitedatabase.h>
#include <sqlitetable.h>
#include <sqlitewritestatement.h>
#include <utf8string.h>

#include <QSignalSpy>
#include <QVariant>

namespace {

using testing::Contains;

using Sqlite::ColumnType;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::Table;

class SqliteDatabase : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    SpyDummy spyDummy;
    QString databaseFilePath = QStringLiteral(":memory:");
    Sqlite::Database database;
};

TEST_F(SqliteDatabase, SetDatabaseFilePath)
{
    ASSERT_THAT(database.databaseFilePath(), databaseFilePath);
}

TEST_F(SqliteDatabase, SetJournalMode)
{
    database.setJournalMode(JournalMode::Memory);

    ASSERT_THAT(database.journalMode(), JournalMode::Memory);
}

TEST_F(SqliteDatabase, SetOpenlMode)
{
    database.setOpenMode(OpenMode::ReadOnly);

    ASSERT_THAT(database.openMode(), OpenMode::ReadOnly);
}

TEST_F(SqliteDatabase, OpenDatabase)
{
    database.close();

    database.open();

    ASSERT_TRUE(database.isOpen());
}

TEST_F(SqliteDatabase, CloseDatabase)
{
    database.close();

    ASSERT_FALSE(database.isOpen());
}

TEST_F(SqliteDatabase, AddTable)
{
    auto sqliteTable = database.addTable();

    ASSERT_THAT(database.tables(), Contains(sqliteTable));
}

TEST_F(SqliteDatabase, GetChangesCount)
{
    Sqlite::WriteStatement statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.changesCount(), 1);
}

TEST_F(SqliteDatabase, GetTotalChangesCount)
{
    Sqlite::WriteStatement statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.lastInsertedRowId(), 1);
}

TEST_F(SqliteDatabase, GetLastInsertedRowId)
{
    Sqlite::WriteStatement statement("INSERT INTO test(name) VALUES (?)", database);
    statement.write(42);

    ASSERT_THAT(database.lastInsertedRowId(), 1);
}

TEST_F(SqliteDatabase, TableIsReadyAfterOpenDatabase)
{
    database.close();
    auto &table = database.addTable();
    table.setName("foo");
    table.addColumn("name");

    database.open();

    ASSERT_TRUE(table.isReady());
}

void SqliteDatabase::SetUp()
{
    database.setJournalMode(JournalMode::Memory);
    database.setDatabaseFilePath(databaseFilePath);
    auto &table = database.addTable();
    table.setName("test");
    table.addColumn("name");

    database.open();
}

void SqliteDatabase::TearDown()
{
    if (database.isOpen())
        database.close();
}
}
