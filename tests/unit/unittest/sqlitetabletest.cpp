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

#include "spydummy.h"

#include <sqlitecolumn.h>
#include <sqlitedatabase.h>
#include <sqlitetable.h>
#include <utf8string.h>

#include <QSignalSpy>
#include <QVariant>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

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
