// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sqlitedatabase.h>
#include <sqlitedatabasebackend.h>
#include <sqliteexception.h>
#include <sqlitetransaction.h>
#include <sqlitewritestatement.h>

#include <3rdparty/sqlite/sqlite.h>

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
        databaseBackend.open(databaseFilePath, OpenMode::ReadWrite, Sqlite::JournalMode::Wal);
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

TEST_F(SqliteDatabaseBackend, open_already_open_database)
{
    ASSERT_THROW(databaseBackend.open(databaseFilePath, OpenMode::ReadWrite, Sqlite::JournalMode::Wal),
                 Sqlite::DatabaseIsAlreadyOpen);
}

TEST_F(SqliteDatabaseBackend, close_already_closed_database)
{
    databaseBackend.close();

    ASSERT_THROW(databaseBackend.close(), Sqlite::DatabaseIsAlreadyClosed);
}

TEST_F(SqliteDatabaseBackend, open_with_wrong_path)
{
    ASSERT_THROW(databaseBackend.open("/xxx/SqliteDatabaseBackendTest.db",
                                      OpenMode::ReadWrite,
                                      Sqlite::JournalMode::Wal),
                 Sqlite::WrongFilePath);
}

TEST_F(SqliteDatabaseBackend, default_journal_mode)
{
    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Delete);
}

TEST_F(SqliteDatabaseBackendSlowTest, wal_journal_mode)
{
    databaseBackend.setJournalMode(JournalMode::Wal);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Wal);
}

TEST_F(SqliteDatabaseBackend, truncate_journal_mode)
{
    databaseBackend.setJournalMode(JournalMode::Truncate);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Truncate);
}

TEST_F(SqliteDatabaseBackend, memory_journal_mode)
{
    databaseBackend.setJournalMode(JournalMode::Memory);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Memory);
}

TEST_F(SqliteDatabaseBackend, persist_journal_mode)
{
    databaseBackend.setJournalMode(JournalMode::Persist);

    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Persist);
}

TEST_F(SqliteDatabaseBackend, open_mode_read_only)
{
    auto mode = Backend::createOpenFlags(OpenMode::ReadOnly, Sqlite::JournalMode::Wal);

    ASSERT_THAT(mode, SQLITE_OPEN_CREATE | SQLITE_OPEN_READONLY | SQLITE_OPEN_EXRESCODE);
}

TEST_F(SqliteDatabaseBackend, open_mode_read_write)
{
    auto mode = Backend::createOpenFlags(OpenMode::ReadWrite, Sqlite::JournalMode::Wal);

    ASSERT_THAT(mode, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_EXRESCODE);
}

TEST_F(SqliteDatabaseBackend, open_mode_read_write_and_memory_journal)
{
    auto mode = Backend::createOpenFlags(OpenMode::ReadWrite, Sqlite::JournalMode::Memory);

    ASSERT_THAT(mode,
                SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_EXRESCODE
                    | SQLITE_OPEN_MEMORY);
}
} // namespace
