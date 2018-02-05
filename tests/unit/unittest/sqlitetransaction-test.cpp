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

#include "mocksqlitetransactionbackend.h"

#include <sqlitetransaction.h>
#include <sqliteexception.h>
#include <mocksqlitedatabase.h>

namespace {

using Sqlite::DeferredTransaction;
using Sqlite::ImmediateTransaction;
using Sqlite::ExclusiveTransaction;
using Sqlite::DeferredNonThrowingDestructorTransaction;
using Sqlite::ImmediateNonThrowingDestructorTransaction;
using Sqlite::ExclusiveNonThrowingDestructorTransaction;

class SqliteTransaction : public testing::Test
{
protected:
    NiceMock<MockSqliteTransactionBackend> mockTransactionBackend;
};

TEST_F(SqliteTransaction, DeferredTransactionCommit)
{
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());

    DeferredTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredTransactionCommitCallsInterface)
{
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());

    DeferredTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredTransactionRollBack)
{
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());

    DeferredTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ImmediateTransactionCommit)
{
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, commit());

    ImmediateTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ImmediateTransactionRollBack)
{
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());

    ImmediateTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ExclusiveTransactionCommit)
{
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, commit());

    ExclusiveTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ExclusiveTransactionRollBack)
{
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());

    ExclusiveTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, DeferredTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, deferredBegin())
            .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(DeferredTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmediateTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, immediateBegin())
            .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(ImmediateTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, ExclusiveTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, exclusiveBegin())
            .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, TransactionCommitThrows)
{
    ON_CALL(mockTransactionBackend, commit())
            .WillByDefault(Throw(Sqlite::Exception("foo")));
    ImmediateTransaction transaction{mockTransactionBackend};

    ASSERT_THROW(transaction.commit(),
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, TransactionRollbackInDestructorThrows)
{
    ON_CALL(mockTransactionBackend, rollback())
            .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, TransactionRollbackInDestructorDontThrows)
{
    ON_CALL(mockTransactionBackend, rollback())
            .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_NO_THROW(ExclusiveNonThrowingDestructorTransaction{mockTransactionBackend});
}

}
