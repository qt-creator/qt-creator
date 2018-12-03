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

#include <utils/temporarydirectory.h>

#include <QSignalSpy>
#include <QTemporaryFile>
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
    void SetUp() override
    {
        database.setJournalMode(JournalMode::Memory);
        database.setDatabaseFilePath(databaseFilePath);
        auto &table = database.addTable();
        table.setName("test");
        table.addColumn("name");

        database.open();
    }

    void TearDown() override
    {
        if (database.isOpen())
            database.close();
    }

    SpyDummy spyDummy;
    QString databaseFilePath{":memory:"};
    Sqlite::Database database;
    Sqlite::TransactionInterface &transactionInterface = database;
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

TEST_F(SqliteDatabase, DatabaseIsNotInitializedAfterOpening)
{
    ASSERT_FALSE(database.isInitialized());
}

TEST_F(SqliteDatabase, DatabaseIsIntializedAfterSettingItBeforeOpening)
{
    database.setIsInitialized(true);

    ASSERT_TRUE(database.isInitialized());
}

TEST_F(SqliteDatabase, DatabaseIsInitializedIfDatabasePathExistsAtOpening)
{
    Sqlite::Database database{TESTDATA_DIR "/sqlite_database.db"};

    ASSERT_TRUE(database.isInitialized());
}

TEST_F(SqliteDatabase, DatabaseIsNotInitializedIfDatabasePathDoesNotExistAtOpening)
{
    Sqlite::Database database{Utils::PathString{Utils::TemporaryDirectory::masterDirectoryPath()
                                                + "/database_does_not_exist.db"}};

    ASSERT_FALSE(database.isInitialized());
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

TEST_F(SqliteDatabase, LastRowId)
{
    database.setLastInsertedRowId(42);

    ASSERT_THAT(database.lastInsertedRowId(), 42);
}

TEST_F(SqliteDatabase, DeferredBegin)
{
    ASSERT_NO_THROW(transactionInterface.deferredBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, ImmediateBegin)
{
    ASSERT_NO_THROW(transactionInterface.immediateBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, ExclusiveBegin)
{
    ASSERT_NO_THROW(transactionInterface.exclusiveBegin());

    transactionInterface.commit();
}

TEST_F(SqliteDatabase, Commit)
{
    transactionInterface.deferredBegin();

    ASSERT_NO_THROW(transactionInterface.commit());
}

TEST_F(SqliteDatabase, Rollback)
{
    transactionInterface.deferredBegin();

    ASSERT_NO_THROW(transactionInterface.rollback());
}

}
