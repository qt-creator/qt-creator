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
#include "mocksqlitedatabase.h"

#include <precompiledheaderstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>
#include <sqlitetransaction.h>

namespace  {

using Storage = ClangBackEnd::PrecompiledHeaderStorage<NiceMock<MockSqliteDatabase>>;

class PrecompiledHeaderStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> database;
    Storage storage{database};
    MockSqliteWriteStatement &insertProjectPrecompiledHeaderStatement = storage.insertProjectPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteProjectPrecompiledHeaderStatement = storage.deleteProjectPrecompiledHeaderStatement;
    MockSqliteWriteStatement &insertSystemPrecompiledHeaderStatement = storage.insertSystemPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteSystemPrecompiledHeaderStatement = storage.deleteSystemPrecompiledHeaderStatement;
    MockSqliteReadStatement &fetchSystemPrecompiledHeaderPathStatement = storage.fetchSystemPrecompiledHeaderPathStatement;
    MockSqliteReadStatement &getPrecompiledHeader = storage.getPrecompiledHeader;
};

TEST_F(PrecompiledHeaderStorage, UseTransaction)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(database, commit());

    Storage storage{database};
}

TEST_F(PrecompiledHeaderStorage, InsertProjectPrecompiledHeader)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPrecompiledHeaderStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertProjectPrecompiledHeader(1, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, InsertProjectPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPrecompiledHeaderStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertProjectPrecompiledHeader(1, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeader)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader(1);
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader(1);
}

TEST_F(PrecompiledHeaderStorage, InsertSystemPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<int>(2),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertSystemPrecompiledHeaders({1, 2}, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, InsertSystemPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<int>(1),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<int>(2),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertSystemPrecompiledHeaders({1, 2}, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, DeleteSystemPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteSystemPrecompiledHeaders({1, 2});
}

TEST_F(PrecompiledHeaderStorage, DeleteSystemPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteSystemPrecompiledHeaders({1, 2});
}

TEST_F(PrecompiledHeaderStorage, CompilePrecompiledHeaderStatements)
{
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};

    ASSERT_NO_THROW(ClangBackEnd::PrecompiledHeaderStorage<>{database});
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderCalls)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement, valueReturnFilePath(TypedEq<int>(1)));
    EXPECT_CALL(database, commit());

    storage.fetchSystemPrecompiledHeaderPath(1);
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeader)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement, valueReturnFilePath(TypedEq<int>(1)))
        .WillOnce(Return(ClangBackEnd::FilePath{"/path/to/pch"}));

    auto path = storage.fetchSystemPrecompiledHeaderPath(1);

    ASSERT_THAT(path, "/path/to/pch");
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderReturnsEmptyPath)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement, valueReturnFilePath(TypedEq<int>(1)))
        .WillOnce(Return(ClangBackEnd::FilePath{}));

    auto path = storage.fetchSystemPrecompiledHeaderPath(1);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderReturnsNullOptional)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement, valueReturnFilePath(TypedEq<int>(1)))
        .WillOnce(Return(Utils::optional<ClangBackEnd::FilePath>{}));

    auto path = storage.fetchSystemPrecompiledHeaderPath(1);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaderCallsValueInStatement)
{
    EXPECT_CALL(getPrecompiledHeader, valueReturnProjectPartPch(Eq(25)));

    storage.fetchPrecompiledHeader(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeader)
{
    ClangBackEnd::ProjectPartPch pch{{}, "/path/to/pch", 131};
    EXPECT_CALL(getPrecompiledHeader, valueReturnProjectPartPch(Eq(25))).WillRepeatedly(Return(pch));

    auto precompiledHeader = storage.fetchPrecompiledHeader(25);

    ASSERT_THAT(precompiledHeader.value(), Eq(pch));
}
}
