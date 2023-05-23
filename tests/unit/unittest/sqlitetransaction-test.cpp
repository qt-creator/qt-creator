// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "mocksqlitetransactionbackend.h"

#include <sqlitedatabasemock.h>
#include <sqliteexception.h>
#include <sqlitetransaction.h>

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
    SqliteTransaction() { ON_CALL(callableWithReturnMock, Call()).WillByDefault(Return(212)); }

protected:
    NiceMock<MockSqliteTransactionBackend> mockTransactionBackend;
    NiceMock<MockFunction<void()>> callableMock;
    NiceMock<MockFunction<int()>> callableWithReturnMock;
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
    ON_CALL(mockTransactionBackend, deferredBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(DeferredTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmediateTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, immediateBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, ExclusiveTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, exclusiveBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, DeferredTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(DeferredTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, ImmediateTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());


    ASSERT_ANY_THROW(ImmediateTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, ExclusiveTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ExclusiveTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, TransactionCommitThrows)
{
    ON_CALL(mockTransactionBackend, commit()).WillByDefault(Throw(Sqlite::Exception()));
    ImmediateTransaction transaction{mockTransactionBackend};

    ASSERT_THROW(transaction.commit(),
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, TransactionRollbackInDestructorThrows)
{
    ON_CALL(mockTransactionBackend, rollback()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend},
                 Sqlite::Exception);
}

TEST_F(SqliteTransaction, TransactionRollbackInDestructorDontThrows)
{
    ON_CALL(mockTransactionBackend, rollback()).WillByDefault(Throw(Sqlite::Exception()));

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
    ON_CALL(mockTransactionBackend, sessionRollback()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmidiateSessionTransactionBeginThrows)
{
    ON_CALL(mockTransactionBackend, immediateSessionBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, ImmediateSessionTransactionBeginThrowsAndNotRollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, sessionRollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ImmediateSessionTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, WithDeferredTransactionNoReturnCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withDeferredTransaction(mockTransactionBackend, callableMock.AsStdFunction());
}

TEST_F(SqliteTransaction, WithDeferredTransactionWithReturnCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(callableWithReturnMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withDeferredTransaction(mockTransactionBackend, callableWithReturnMock.AsStdFunction());
}

TEST_F(SqliteTransaction, WithDeferredTransactionReturnsValue)
{
    auto callable = callableWithReturnMock.AsStdFunction();

    auto value = Sqlite::withDeferredTransaction(mockTransactionBackend,
                                                 callableWithReturnMock.AsStdFunction());

    ASSERT_THAT(value, Eq(212));
}

TEST_F(SqliteTransaction, WithDeferredTransactionRollsbackForException)
{
    InSequence s;
    ON_CALL(callableMock, Call()).WillByDefault(Throw(std::exception{}));

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    try {
        Sqlite::withDeferredTransaction(mockTransactionBackend, callableMock.AsStdFunction());
    } catch (...) {
    }
}

TEST_F(SqliteTransaction, WithImmediateTransactionNoReturnCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImmediateTransaction(mockTransactionBackend, callableMock.AsStdFunction());
}

TEST_F(SqliteTransaction, WithImmediateTransactionWithReturnCommit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(callableWithReturnMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImmediateTransaction(mockTransactionBackend, callableWithReturnMock.AsStdFunction());
}

TEST_F(SqliteTransaction, WithImmediateTransactionReturnsValue)
{
    auto callable = callableWithReturnMock.AsStdFunction();

    auto value = Sqlite::withImmediateTransaction(mockTransactionBackend,
                                                  callableWithReturnMock.AsStdFunction());

    ASSERT_THAT(value, Eq(212));
}

TEST_F(SqliteTransaction, WithImmediateTransactionRollsbackForException)
{
    InSequence s;
    ON_CALL(callableMock, Call()).WillByDefault(Throw(std::exception{}));

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    try {
        Sqlite::withImmediateTransaction(mockTransactionBackend, callableMock.AsStdFunction());
    } catch (...) {
    }
}

} // namespace
