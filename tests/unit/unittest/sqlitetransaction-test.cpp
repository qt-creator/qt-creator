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

using Sqlite::DeferredNonThrowingDestructorTransaction;
using Sqlite::DeferredTransaction;
using Sqlite::ExclusiveNonThrowingDestructorTransaction;
using Sqlite::ExclusiveTransaction;
using Sqlite::ImmediateNonThrowingDestructorTransaction;
using Sqlite::ImmediateSessionTransaction;
using Sqlite::ImmediateTransaction;

class SqliteTransaction : public testing::Test
{
protected:
    NiceMock<MockSqliteTransactionBackend> mockTransactionBackend;
};

TEST_F(SqliteTransaction, DeferredTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredTransactionCommitCallsInterface)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ImmediateTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ImmediateTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ExclusiveTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ExclusiveTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, DeferredNonThrowingDestructorTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredNonThrowingDestructorTransactionCommitCallsInterface)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, DeferredNonThrowingDestructorTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ImmediateNonThrowingDestructorTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ImmediateNonThrowingDestructorTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateNonThrowingDestructorTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, ExclusiveNonThrowingDestructorTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ExclusiveTNonThrowingDestructorransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveNonThrowingDestructorTransaction transaction{mockTransactionBackend};
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

TEST_F(SqliteTransaction, DeferredTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin())
            .WillOnce(Throw(Sqlite::Exception("foo")));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(DeferredTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, ImmediateTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin())
            .WillOnce(Throw(Sqlite::Exception("foo")));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());


    ASSERT_ANY_THROW(ImmediateTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, ExclusiveTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin())
            .WillOnce(Throw(Sqlite::Exception("foo")));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ExclusiveTransaction{mockTransactionBackend});
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

TEST_F(SqliteTransaction, ImmediateSessionTransactionCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin());
    EXPECT_CALL(mockTransactionBackend, sessionCommit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateSessionTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, ImmediateSessionTransactionRollBack)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin());
    EXPECT_CALL(mockTransactionBackend, sessionRollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateSessionTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, SessionTransactionRollbackInDestructorThrows)
{
    ON_CALL(mockTransactionBackend, sessionRollback()).WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmidiateSessionTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, immediateSessionBegin())
        .WillByDefault(Throw(Sqlite::Exception("foo")));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmediateSessionTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin()).WillOnce(Throw(Sqlite::Exception("foo")));
    EXPECT_CALL(mockTransactionBackend, sessionRollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ImmediateSessionTransaction{mockTransactionBackend});
}

} // namespace
