// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/mocksqlitetransactionbackend.h"
#include "../mocks/sqlitedatabasemock.h"

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

TEST_F(SqliteTransaction, deferred_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, deferred_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, immediate_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, immediate_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, exclusive_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, exclusive_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, deferred_non_throwing_destructor_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, deferred_non_throwing_destructor_transaction_commit_calls_interface)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, deferred_non_throwing_destructor_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    DeferredNonThrowingDestructorTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, immediate_non_throwing_destructor_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, immediate_non_throwing_destructor_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateNonThrowingDestructorTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, exclusive_non_throwing_destructor_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveNonThrowingDestructorTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, exclusive_t_non_throwing_destructorransaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin());
    EXPECT_CALL(mockTransactionBackend, rollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ExclusiveNonThrowingDestructorTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, deferred_transaction_begin_throws)
{
    ON_CALL(mockTransactionBackend, deferredBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(DeferredTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, immediate_transaction_begin_throws)
{
    ON_CALL(mockTransactionBackend, immediateBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, exclusive_transaction_begin_throws)
{
    ON_CALL(mockTransactionBackend, exclusiveBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, deferred_transaction_begin_throws_and_not_rollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(DeferredTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, immediate_transaction_begin_throws_and_not_rollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ImmediateTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, exclusive_transaction_begin_throws_and_not_rollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, exclusiveBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ExclusiveTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, transaction_commit_throws)
{
    ON_CALL(mockTransactionBackend, commit()).WillByDefault(Throw(Sqlite::Exception()));
    ImmediateTransaction transaction{mockTransactionBackend};

    ASSERT_THROW(transaction.commit(), Sqlite::Exception);
}

TEST_F(SqliteTransaction, transaction_rollback_in_destructor_throws)
{
    ON_CALL(mockTransactionBackend, rollback()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ExclusiveTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, transaction_rollback_in_destructor_dont_throws)
{
    ON_CALL(mockTransactionBackend, rollback()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_NO_THROW(ExclusiveNonThrowingDestructorTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, immediate_session_transaction_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin());
    EXPECT_CALL(mockTransactionBackend, sessionCommit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateSessionTransaction transaction{mockTransactionBackend};
    transaction.commit();
}

TEST_F(SqliteTransaction, immediate_session_transaction_roll_back)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin());
    EXPECT_CALL(mockTransactionBackend, sessionRollback());
    EXPECT_CALL(mockTransactionBackend, unlock());

    ImmediateSessionTransaction transaction{mockTransactionBackend};
}

TEST_F(SqliteTransaction, session_transaction_rollback_in_destructor_throws)
{
    ON_CALL(mockTransactionBackend, sessionRollback()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, immidiate_session_transaction_begin_throws)
{
    ON_CALL(mockTransactionBackend, immediateSessionBegin()).WillByDefault(Throw(Sqlite::Exception()));

    ASSERT_THROW(ImmediateSessionTransaction{mockTransactionBackend}, Sqlite::Exception);
}

TEST_F(SqliteTransaction, immediate_session_transaction_begin_throws_and_not_rollback)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateSessionBegin()).WillOnce(Throw(Sqlite::Exception()));
    EXPECT_CALL(mockTransactionBackend, sessionRollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    ASSERT_ANY_THROW(ImmediateSessionTransaction{mockTransactionBackend});
}

TEST_F(SqliteTransaction, with_implicit_transaction_no_return_does_not_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin()).Times(0);
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImplicitTransaction(mockTransactionBackend, callableMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_implicit_transaction_with_return_does_not_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin()).Times(0);
    EXPECT_CALL(callableWithReturnMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImplicitTransaction(mockTransactionBackend, callableWithReturnMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_implicit_transaction_returns_value)
{
    auto callable = callableWithReturnMock.AsStdFunction();

    auto value = Sqlite::withImplicitTransaction(mockTransactionBackend,
                                                 callableWithReturnMock.AsStdFunction());

    ASSERT_THAT(value, Eq(212));
}

TEST_F(SqliteTransaction, with_implicit_transaction_do_calls_rollsback_for_exception)
{
    InSequence s;
    ON_CALL(callableMock, Call()).WillByDefault(Throw(std::exception{}));

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin()).Times(0);
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, rollback()).Times(0);
    EXPECT_CALL(mockTransactionBackend, unlock());

    try {
        Sqlite::withImplicitTransaction(mockTransactionBackend, callableMock.AsStdFunction());
    } catch (...) {
    }
}

TEST_F(SqliteTransaction, with_deferred_transaction_no_return_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withDeferredTransaction(mockTransactionBackend, callableMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_deferred_transaction_with_return_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, deferredBegin());
    EXPECT_CALL(callableWithReturnMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withDeferredTransaction(mockTransactionBackend, callableWithReturnMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_deferred_transaction_returns_value)
{
    auto callable = callableWithReturnMock.AsStdFunction();

    auto value = Sqlite::withDeferredTransaction(mockTransactionBackend,
                                                 callableWithReturnMock.AsStdFunction());

    ASSERT_THAT(value, Eq(212));
}

TEST_F(SqliteTransaction, with_deferred_transaction_rollsback_for_exception)
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

TEST_F(SqliteTransaction, with_immediate_transaction_no_return_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(callableMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImmediateTransaction(mockTransactionBackend, callableMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_immediate_transaction_with_return_commit)
{
    InSequence s;

    EXPECT_CALL(mockTransactionBackend, lock());
    EXPECT_CALL(mockTransactionBackend, immediateBegin());
    EXPECT_CALL(callableWithReturnMock, Call());
    EXPECT_CALL(mockTransactionBackend, commit());
    EXPECT_CALL(mockTransactionBackend, unlock());

    Sqlite::withImmediateTransaction(mockTransactionBackend, callableWithReturnMock.AsStdFunction());
}

TEST_F(SqliteTransaction, with_immediate_transaction_returns_value)
{
    auto callable = callableWithReturnMock.AsStdFunction();

    auto value = Sqlite::withImmediateTransaction(mockTransactionBackend,
                                                  callableWithReturnMock.AsStdFunction());

    ASSERT_THAT(value, Eq(212));
}

TEST_F(SqliteTransaction, with_immediate_transaction_rollsback_for_exception)
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
