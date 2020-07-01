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
    MockSqliteWriteStatement &deleteProjectPrecompiledHeaderPathAndSetBuildTimeStatement
        = storage.deleteProjectPrecompiledHeaderPathAndSetBuildTimeStatement;
    MockSqliteWriteStatement &insertSystemPrecompiledHeaderStatement = storage.insertSystemPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteSystemPrecompiledHeaderStatement = storage.deleteSystemPrecompiledHeaderStatement;
    MockSqliteWriteStatement &deleteSystemAndProjectPrecompiledHeaderStatement = storage.deleteSystemAndProjectPrecompiledHeaderStatement;
    MockSqliteReadStatement &fetchSystemPrecompiledHeaderPathStatement = storage.fetchSystemPrecompiledHeaderPathStatement;
    MockSqliteReadStatement &fetchPrecompiledHeaderStatement = storage.fetchPrecompiledHeaderStatement;
    MockSqliteReadStatement &fetchPrecompiledHeadersStatement = storage.fetchPrecompiledHeadersStatement;
    MockSqliteReadStatement &fetchTimeStampsStatement = storage.fetchTimeStampsStatement;
    MockSqliteReadStatement &fetchAllPchPathsStatement = storage.fetchAllPchPathsStatement;
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
    EXPECT_CALL(deleteProjectPrecompiledHeaderPathAndSetBuildTimeStatement,
                write(TypedEq<int>(1), TypedEq<long long>(13)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader(1, 13);
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderPathAndSetBuildTimeStatement,
                write(TypedEq<int>(1), TypedEq<long long>(13)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeader(1, 13);
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeaders({1, 2});
}

TEST_F(PrecompiledHeaderStorage, DeleteProjectPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteProjectPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteProjectPrecompiledHeaders({1, 2});
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

TEST_F(PrecompiledHeaderStorage, DeleteSystemAndProjectPrecompiledHeaders)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemAndProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteSystemAndProjectPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteSystemAndProjectPrecompiledHeaders({1, 2});
}

TEST_F(PrecompiledHeaderStorage, DeleteSystemAndProjectPrecompiledHeadersStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deleteSystemAndProjectPrecompiledHeaderStatement, write(TypedEq<int>(1)));
    EXPECT_CALL(deleteSystemAndProjectPrecompiledHeaderStatement, write(TypedEq<int>(2)));
    EXPECT_CALL(database, commit());

    storage.deleteSystemAndProjectPrecompiledHeaders({1, 2});
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

TEST_F(PrecompiledHeaderStorage, FetchSystemPrecompiledHeaderCallsWithReturnValue)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchSystemPrecompiledHeaderPathStatement, valueReturnFilePath(TypedEq<int>(1)))
        .WillOnce(Return(ClangBackEnd::FilePath{}));
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
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeaderStatement, valueReturnFilePath(Eq(25)));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeader(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaderCallsWithValue)
{
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeaderStatement, valueReturnFilePath(Eq(25)))
        .WillOnce(Return(ClangBackEnd::FilePath{}));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeader(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaderIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeaderStatement, valueReturnFilePath(Eq(25)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(database, rollback());
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeaderStatement, valueReturnFilePath(Eq(25)));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeader(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeader)
{
    ClangBackEnd::FilePath pchFilePath{"/path/to/pch"};
    ON_CALL(fetchPrecompiledHeaderStatement, valueReturnFilePath(Eq(25)))
        .WillByDefault(Return(pchFilePath));

    auto path = storage.fetchPrecompiledHeader(25);

    ASSERT_THAT(path, Eq(pchFilePath));
}

TEST_F(PrecompiledHeaderStorage, FetchEmptyPrecompiledHeader)
{
    auto path = storage.fetchPrecompiledHeader(25);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaderCalls)
{
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeadersStatement, valueReturnPchPaths(Eq(25)));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeaders(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaderCallsWithReturnValue)
{
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeadersStatement, valueReturnPchPaths(Eq(25)))
        .WillOnce(Return(ClangBackEnd::PchPaths{}));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeaders(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeadersIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeadersStatement, valueReturnPchPaths(Eq(25)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(database, rollback());
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchPrecompiledHeadersStatement, valueReturnPchPaths(Eq(25)));
    EXPECT_CALL(database, commit());

    storage.fetchPrecompiledHeaders(25);
}

TEST_F(PrecompiledHeaderStorage, FetchPrecompiledHeaders)
{
    ClangBackEnd::PchPaths pchFilePaths{"/project/pch", "/system/pch"};
    ON_CALL(fetchPrecompiledHeadersStatement, valueReturnPchPaths(Eq(25)))
        .WillByDefault(Return(pchFilePaths));

    auto paths = storage.fetchPrecompiledHeaders(25);

    ASSERT_THAT(paths, Eq(pchFilePaths));
}

TEST_F(PrecompiledHeaderStorage, FetchEmptyPrecompiledHeaders)
{
    auto paths = storage.fetchPrecompiledHeaders(25);

    ASSERT_THAT(paths,
                AllOf(Field(&ClangBackEnd::PchPaths::projectPchPath, IsEmpty()),
                      Field(&ClangBackEnd::PchPaths::systemPchPath, IsEmpty())));
}

TEST_F(PrecompiledHeaderStorage, FetchTimeStamps)
{
    ClangBackEnd::PrecompiledHeaderTimeStamps precompiledHeaderTimeStamps{22, 33};
    ON_CALL(fetchTimeStampsStatement, valuesReturnPrecompiledHeaderTimeStamps(Eq(23)))
        .WillByDefault(Return(precompiledHeaderTimeStamps));

    auto timeStamps = storage.fetchTimeStamps(23);

    ASSERT_THAT(timeStamps,
                AllOf(Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::project, Eq(22)),
                      Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::system, Eq(33))));
}

TEST_F(PrecompiledHeaderStorage, NoFetchTimeStamps)
{
    auto timeStamps = storage.fetchTimeStamps(23);

    ASSERT_THAT(timeStamps,
                AllOf(Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::project, Eq(-1)),
                      Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::system, Eq(-1))));
}

TEST_F(PrecompiledHeaderStorage, FetchTimeStampsCalls)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchTimeStampsStatement, valuesReturnPrecompiledHeaderTimeStamps(Eq(23)));
    EXPECT_CALL(database, commit());

    storage.fetchTimeStamps(23);
}

TEST_F(PrecompiledHeaderStorage, FetchTimeStampsCallsWithReturnValue)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchTimeStampsStatement, valuesReturnPrecompiledHeaderTimeStamps(Eq(23)))
        .WillOnce(Return(ClangBackEnd::PrecompiledHeaderTimeStamps{}));
    EXPECT_CALL(database, commit());

    storage.fetchTimeStamps(23);
}

TEST_F(PrecompiledHeaderStorage, FetchTimeStampsBusy)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchTimeStampsStatement, valuesReturnPrecompiledHeaderTimeStamps(Eq(23)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(database, rollback());
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchTimeStampsStatement, valuesReturnPrecompiledHeaderTimeStamps(Eq(23)));
    EXPECT_CALL(database, commit());

    storage.fetchTimeStamps(23);
}

TEST_F(PrecompiledHeaderStorage, FetchAllPchPaths)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchAllPchPathsStatement, valuesReturnFilePaths(_));
    EXPECT_CALL(database, commit());

    storage.fetchAllPchPaths();
}

TEST_F(PrecompiledHeaderStorage, FetchAllPchPathsIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchAllPchPathsStatement, valuesReturnFilePaths(_))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(database, rollback());
    EXPECT_CALL(database, deferredBegin());
    EXPECT_CALL(fetchAllPchPathsStatement, valuesReturnFilePaths(_));
    EXPECT_CALL(database, commit());

    storage.fetchAllPchPaths();
}

class PrecompiledHeaderStorageSlowTest : public testing::Test
{
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::PrecompiledHeaderStorage<> storage{database};
};

TEST_F(PrecompiledHeaderStorageSlowTest, NoFetchTimeStamps)
{
    storage.insertProjectPrecompiledHeader(23, {}, 22);
    storage.insertSystemPrecompiledHeaders({23}, {}, 33);

    auto timeStamps = storage.fetchTimeStamps(23);

    ASSERT_THAT(timeStamps,
                AllOf(Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::project, Eq(22)),
                      Field(&ClangBackEnd::PrecompiledHeaderTimeStamps::system, Eq(33))));
}

TEST_F(PrecompiledHeaderStorageSlowTest, FetchAllPchPaths)
{
    storage.insertProjectPrecompiledHeader(11, "/tmp/yi", 22);
    storage.insertProjectPrecompiledHeader(12, "/tmp/er", 22);
    storage.insertSystemPrecompiledHeaders({11, 12}, "/tmp/se", 33);
    storage.insertSystemPrecompiledHeaders({13}, "/tmp/wu", 33);
    storage.insertProjectPrecompiledHeader(13, "/tmp/san", 22);

    auto filePathIds = storage.fetchAllPchPaths();

    ASSERT_THAT(filePathIds,
                UnorderedElementsAre("/tmp/er", "/tmp/san", "/tmp/se", "/tmp/wu", "/tmp/yi"));
}

} // namespace
