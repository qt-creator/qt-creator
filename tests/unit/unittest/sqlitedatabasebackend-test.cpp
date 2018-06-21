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

#include <sqlitedatabase.h>
#include <sqlitedatabasebackend.h>
#include <sqliteexception.h>
#include <sqlitewritestatement.h>

#include <sqlite3.h>

#include <QDir>

namespace {

using Backend = Sqlite::DatabaseBackend;

using Sqlite::ColumnType;
using Sqlite::Contraint;
using Sqlite::JournalMode;
using Sqlite::OpenMode;
using Sqlite::TextEncoding;
using Sqlite::Exception;
using Sqlite::WriteStatement;

class SqliteDatabaseBackend : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

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

TEST_F(SqliteDatabaseBackend, DefaultTextEncoding)
{
    ASSERT_THAT(databaseBackend.textEncoding(), TextEncoding::Utf8);
}

TEST_F(SqliteDatabaseBackend, Utf16TextEncoding)
{
    databaseBackend.setTextEncoding(TextEncoding::Utf16);

    ASSERT_THAT(databaseBackend.textEncoding(), TextEncoding::Utf16);
}

TEST_F(SqliteDatabaseBackend, Utf16beTextEncoding)
{
    databaseBackend.setTextEncoding(TextEncoding::Utf16be);

    ASSERT_THAT(databaseBackend.textEncoding(),TextEncoding::Utf16be);
}

TEST_F(SqliteDatabaseBackend, Utf16leTextEncoding)
{
    databaseBackend.setTextEncoding(TextEncoding::Utf16le);

    ASSERT_THAT(databaseBackend.textEncoding(), TextEncoding::Utf16le);
}

TEST_F(SqliteDatabaseBackend, Utf8TextEncoding)
{
    databaseBackend.setTextEncoding(TextEncoding::Utf8);

    ASSERT_THAT(databaseBackend.textEncoding(), TextEncoding::Utf8);
}

TEST_F(SqliteDatabaseBackend, TextEncodingCannotBeChangedAfterTouchingDatabase)
{
    databaseBackend.setJournalMode(JournalMode::Memory);

    databaseBackend.execute("CREATE TABLE text(name, number)");

    ASSERT_THROW(databaseBackend.setTextEncoding(TextEncoding::Utf16),
                 Sqlite::PragmaValueNotSet);
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

void SqliteDatabaseBackend::SetUp()
{
    QDir::temp().remove(QStringLiteral("SqliteDatabaseBackendTest.db"));
    databaseBackend.open(databaseFilePath, OpenMode::ReadWrite);
}

void SqliteDatabaseBackend::TearDown()
{
    databaseBackend.closeWithoutException();
}
}
