/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <sqlitetransaction.h>
#include <mocksqlitedatabase.h>

namespace {

using DeferredTransaction = Sqlite::DeferredTransaction<MockSqliteDatabase>;
using ImmediateTransaction = Sqlite::ImmediateTransaction<MockSqliteDatabase>;
using ExclusiveTransaction = Sqlite::ExclusiveTransaction<MockSqliteDatabase>;

class SqliteTransaction : public testing::Test
{
protected:
    MockMutex mockMutex;
    MockSqliteDatabase mockDatabase{mockMutex};
};

TEST_F(SqliteTransaction, DeferredTransactionCommit)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));
    EXPECT_CALL(mockMutex, unlock());

    DeferredTransaction transaction{mockDatabase};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredTransactionRollBack)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(mockDatabase, execute(Eq("ROLLBACK")));
    EXPECT_CALL(mockMutex, unlock());

    DeferredTransaction transaction{mockDatabase};
}

TEST_F(SqliteTransaction, ImmediateTransactionCommit)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN IMMEDIATE")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));
    EXPECT_CALL(mockMutex, unlock());

    ImmediateTransaction transaction{mockDatabase};
    transaction.commit();
}

TEST_F(SqliteTransaction, ImmediateTransactionRollBack)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN IMMEDIATE")));
    EXPECT_CALL(mockDatabase, execute(Eq("ROLLBACK")));
    EXPECT_CALL(mockMutex, unlock());

    ImmediateTransaction transaction{mockDatabase};
}

TEST_F(SqliteTransaction, ExclusiveTransactionCommit)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN EXCLUSIVE")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));
    EXPECT_CALL(mockMutex, unlock());

    ExclusiveTransaction transaction{mockDatabase};
    transaction.commit();
}

TEST_F(SqliteTransaction, ExclusiveTransactionRollBack)
{
    EXPECT_CALL(mockDatabase, databaseMutex());
    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN EXCLUSIVE")));
    EXPECT_CALL(mockDatabase, execute(Eq("ROLLBACK")));
    EXPECT_CALL(mockMutex, unlock());

    ExclusiveTransaction transaction{mockDatabase};
}

}


