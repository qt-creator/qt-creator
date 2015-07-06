/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sqlitedatabasebackend.h"
#include "sqliteexception.h"
#include "sqlitewritestatement.h"

#include <QDir>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

class SqliteDatabaseBackend : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    QString databaseFilePath = QDir::tempPath() + QStringLiteral("/SqliteDatabaseBackendTest.db");
    ::SqliteDatabaseBackend databaseBackend;
};

TEST_F(SqliteDatabaseBackend, OpenAlreadyOpenDatabase)
{
    ASSERT_THROW(databaseBackend.open(databaseFilePath), SqliteException);
}

TEST_F(SqliteDatabaseBackend, CloseAlreadyClosedDatabase)
{
    databaseBackend.close();
    ASSERT_THROW(databaseBackend.close(), SqliteException);
}

TEST_F(SqliteDatabaseBackend, OpenWithWrongPath)
{
    ASSERT_THROW(databaseBackend.open(QStringLiteral("/xxx/SqliteDatabaseBackendTest.db")), SqliteException);
}

TEST_F(SqliteDatabaseBackend, DefaultJournalMode)
{
    ASSERT_THAT(databaseBackend.journalMode(), JournalMode::Delete);
}

TEST_F(SqliteDatabaseBackend, WalJournalMode)
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
    ASSERT_THAT(databaseBackend.textEncoding(), Utf8);
}

TEST_F(SqliteDatabaseBackend, Utf16TextEncoding)
{
    databaseBackend.setTextEncoding(Utf16);

    ASSERT_THAT(databaseBackend.textEncoding(), Utf16);
}

TEST_F(SqliteDatabaseBackend, Utf16beTextEncoding)
{
    databaseBackend.setTextEncoding(Utf16be);

    ASSERT_THAT(databaseBackend.textEncoding(), Utf16be);
}

TEST_F(SqliteDatabaseBackend, Utf16leTextEncoding)
{
    databaseBackend.setTextEncoding(Utf16le);

    ASSERT_THAT(databaseBackend.textEncoding(), Utf16le);
}

TEST_F(SqliteDatabaseBackend, Utf8TextEncoding)
{
    databaseBackend.setTextEncoding(Utf8);

    ASSERT_THAT(databaseBackend.textEncoding(), Utf8);
}

TEST_F(SqliteDatabaseBackend, TextEncodingCannotBeChangedAfterTouchingDatabase)
{
    databaseBackend.setJournalMode(JournalMode::Memory);

    SqliteWriteStatement::execute(Utf8StringLiteral("CREATE TABLE text(name, number)"));

    ASSERT_THROW(databaseBackend.setTextEncoding(Utf16), SqliteException);
}

void SqliteDatabaseBackend::SetUp()
{
    QDir::temp().remove(QStringLiteral("SqliteDatabaseBackendTest.db"));
    databaseBackend.open(databaseFilePath);
}

void SqliteDatabaseBackend::TearDown()
{
    databaseBackend.closeWithoutException();
}
}
