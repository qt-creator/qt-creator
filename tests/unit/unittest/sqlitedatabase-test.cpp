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
#include <utf8string.h>

#include <QSignalSpy>
#include <QVariant>

namespace {

class SqliteDatabase : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    SpyDummy spyDummy;
    QString databaseFilePath = QStringLiteral(":memory:");
    ::SqliteDatabase database;
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

TEST_F(SqliteDatabase, OpenDatabase)
{
    database.close();
    QSignalSpy signalSpy(&spyDummy, &SpyDummy::databaseIsOpened);
    database.open();

    ASSERT_TRUE(signalSpy.wait(100000));
    ASSERT_TRUE(database.isOpen());
}

TEST_F(SqliteDatabase, CloseDatabase)
{
    QSignalSpy signalSpy(&spyDummy, &SpyDummy::databaseIsClosed);

    database.close();

    ASSERT_TRUE(signalSpy.wait(100000));
    ASSERT_FALSE(database.isOpen());
}

TEST_F(SqliteDatabase, AddTable)
{
    SqliteTable *sqliteTable = new SqliteTable;

    database.addTable(sqliteTable);

    ASSERT_THAT(database.tables().first(), sqliteTable);
}

void SqliteDatabase::SetUp()
{
    QObject::connect(&database, &::SqliteDatabase::databaseIsOpened, &spyDummy, &SpyDummy::databaseIsOpened);
    QObject::connect(&database, &::SqliteDatabase::databaseIsClosed, &spyDummy, &SpyDummy::databaseIsClosed);

    database.setJournalMode(JournalMode::Memory);
    database.setDatabaseFilePath(databaseFilePath);
}

void SqliteDatabase::TearDown()
{
    database.close();
}
}
