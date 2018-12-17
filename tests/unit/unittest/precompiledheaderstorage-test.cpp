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
    MockSqliteWriteStatement &insertProjectPartStatement = storage.m_insertProjectPartStatement;
    MockSqliteWriteStatement &insertProjectPrecompiledHeaderStatement
        = storage.m_insertProjectPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteProjectPrecompiledHeaderStatement
        = storage.m_deleteProjectPrecompiledHeaderStatement;
    MockSqliteWriteStatement &insertSystemPrecompiledHeaderStatement
        = storage.m_insertSystemPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteSystemPrecompiledHeaderStatement
        = storage.m_deleteSystemPrecompiledHeaderStatement;
    MockSqliteReadStatement &fetchSystemPrecompiledHeaderPathStatement
        = storage.m_fetchSystemPrecompiledHeaderPathStatement;
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
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertProjectPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertProjectPrecompiledHeader("project1", "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, InsertProjectPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertProjectPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertProjectPrecompiledHeader("project1", "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeader)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader("project1");
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader("project1");
}

TEST_F(PrecompiledHeaderStorage, InsertSystemPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project2")));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project2"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertSystemPrecompiledHeaders({"project1", "project2"}, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, InsertSystemPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project2")));
    EXPECT_CALL(insertSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project2"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertSystemPrecompiledHeaders({"project1", "project2"}, "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, DeleteSystemPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project2")));
    EXPECT_CALL(database, commit());

    storage.deleteSystemPrecompiledHeaders({"project1", "project2"});
}

TEST_F(PrecompiledHeaderStorage, DeleteSystemPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(deleteSystemPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project2")));
    EXPECT_CALL(database, commit());

    storage.deleteSystemPrecompiledHeaders({"project1", "project2"});
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
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement,
                valueReturnFilePath(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(database, commit());

    storage.fetchSystemPrecompiledHeaderPath("project1");
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeader)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement,
                valueReturnFilePath(TypedEq<Utils::SmallStringView>("project1")))
        .WillOnce(Return(ClangBackEnd::FilePath{"/path/to/pch"}));

    auto path = storage.fetchSystemPrecompiledHeaderPath("project1");

    ASSERT_THAT(path, "/path/to/pch");
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderReturnsEmptyPath)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement,
                valueReturnFilePath(TypedEq<Utils::SmallStringView>("project1")))
        .WillOnce(Return(ClangBackEnd::FilePath{}));

    auto path = storage.fetchSystemPrecompiledHeaderPath("project1");

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderReturnsNullOptional)
{
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement,
                valueReturnFilePath(TypedEq<Utils::SmallStringView>("project1")))
        .WillOnce(Return(Utils::optional<ClangBackEnd::FilePath>{}));

    auto path = storage.fetchSystemPrecompiledHeaderPath("project1");

    ASSERT_THAT(path, IsEmpty());
}
}
