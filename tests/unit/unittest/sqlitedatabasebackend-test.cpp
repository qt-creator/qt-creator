// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <sqlitedatabase.h>
#include <sqlitedatabasebackend.h>
#include <sqliteexception.h>
#include <sqlitetransaction.h>
#include <sqlitewritestatement.h>

#include <sqlite.h>

#include <QDir>

namespace {

using Backend = Sqlite::DatabaseBackend;

using Sqlite::ColumnType;
using Sqlite::ConstraintType;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::Exception;
using Sqlite::WriteStatement;

class SqliteDatabaseBackend : public ::testing::Test
{
protected:
    SqliteDatabaseBackend()
    {
        database.lock();
        QDir::temp().remove(QStringLiteral("SqliteDatabaseBackendTest.db"));
        databaseBackend.open(databaseFilePath, OpenMode::ReadWrite);
    }

    ~SqliteDatabaseBackend() noexcept(true)
    {
        databaseBackend.closeWithoutException();
        database.unlock();
    }

    Utils::PathString databaseFilePath = UnitTest::temporaryDirPath() + "/SqliteDatabaseBackendTest.db";
    Sqlite::Database database;
    Sqlite::DatabaseBackend &databaseBackend = database.backend();
};

using SqliteDatabaseBackendSlowTest = SqliteDatabaseBackend;

TEST_F(SqliteDatabaseBackend, OpenAlreadyOpenDatabase)
{
    ASSERT_THROW(databaseBackend.open(databaseFilePath, OpenMode::ReadWrite),
                 Sqlite::DatabaseIsAlreadyOpen);
}

TEST_F(SqliteDatabaseBackend, CloseAlreadyClosedDatabase)
{
    databaseBackend.close();

    ASSERT_THROW(databaseBackend.close(), Sqlite::DatabaseIsAlreadyClosed);
}

TEST_F(SqliteDatabaseBackend, OpenWithWrongPath)
{
    ASSERT_THROW(databaseBackend.open("/xxx/SqliteDatabaseBackendTest.db", OpenMode::ReadWrite),
                 Sqlite::WrongFilePath);
}

TEST_F(SqliteDatabaseBackend, DefaultJournalMode)
{
    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Delete);
}

TEST_F(SqliteDatabaseBackendSlowTest, WalJournalMode)
{
    databaseBackend.setJournalMode(JournalMode::Wal);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Wal);
}

TEST_F(SqliteDatabaseBackend, TruncateJournalMode)
{
    databaseBackend.setJournalMode(JournalMode::Truncate);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Truncate);
}

TEST_F(SqliteDatabaseBackend, MemoryJournalMode)
{
    databaseBackend.setJournalMode(JournalMode::Memory);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Memory);
}

TEST_F(SqliteDatabaseBackend, PersistJournalMode)
{
    databaseBackend.setJournalMode(JournalMode::Persist);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Persist);
}

TEST_F(SqliteDatabaseBackend, OpenModeReadOnly)
{
    auto mode = Backend::openMode(OpenMode::ReadOnly);

    ASSERT_THAT(mode, SQLITE_OPEN_CREATE | SQLITE_OPEN_READONLY);
}

TEST_F(SqliteDatabaseBackend, OpenModeReadWrite)
{
    auto mode = Backend::openMode(OpenMode::ReadWrite);

    ASSERT_THAT(mode, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
}
} // namespace
