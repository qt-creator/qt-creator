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
#include <sqlitewritestatement.h>
#include <sqlitetransaction.h>

namespace  {

using Storage = ClangBackEnd::PrecompiledHeaderStorage<NiceMock<MockSqliteDatabase>>;

class PrecompiledHeaderStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> database;
    Storage storage{database};
    MockSqliteWriteStatement &insertPrecompiledHeaderStatement  = storage.m_insertPrecompiledHeaderStatement;
    MockSqliteWriteStatement &insertProjectPartStatement = storage.m_insertProjectPartStatement;
    MockSqliteWriteStatement &deletePrecompiledHeaderStatement  = storage.m_deletePrecompiledHeaderStatement;
};

TEST_F(PrecompiledHeaderStorage, UseTransaction)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(database, commit());

    Storage storage{database};
}

TEST_F(PrecompiledHeaderStorage, InsertPrecompiledHeader)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertPrecompiledHeader("project1", "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, InsertPrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(insertProjectPartStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(insertPrecompiledHeaderStatement,
                write(TypedEq<Utils::SmallStringView>("project1"),
                      TypedEq<Utils::SmallStringView>("/path/to/pch"),
                      TypedEq<long long>(22)));
    EXPECT_CALL(database, commit());

    storage.insertPrecompiledHeader("project1", "/path/to/pch", 22);
}

TEST_F(PrecompiledHeaderStorage, DeletePrecompiledHeader)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deletePrecompiledHeaderStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(database, commit());

    storage.deletePrecompiledHeader("project1");
}

TEST_F(PrecompiledHeaderStorage, DeletePrecompiledHeaderStatementIsBusy)
{
    InSequence s;

    EXPECT_CALL(database, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(database, immediateBegin());
    EXPECT_CALL(deletePrecompiledHeaderStatement, write(TypedEq<Utils::SmallStringView>("project1")));
    EXPECT_CALL(database, commit());

    storage.deletePrecompiledHeader("project1");
}

TEST_F(PrecompiledHeaderStorage, InsertPrecompiledHeaderStatement)
{
    ASSERT_THAT(insertPrecompiledHeaderStatement.sqlStatement,
                Eq("INSERT OR REPLACE INTO precompiledHeaders(projectPartId, pchPath, pchBuildTime) VALUES((SELECT projectPartId FROM projectParts WHERE projectPartName = ?),?,?)"));
}

TEST_F(PrecompiledHeaderStorage, InsertProjectPartStatement)
{
    ASSERT_THAT(insertProjectPartStatement.sqlStatement,
                Eq("INSERT OR IGNORE INTO projectParts(projectPartName) VALUES (?)"));
}

TEST_F(PrecompiledHeaderStorage, DeletePrecompiledHeaderStatement)
{
    ASSERT_THAT(deletePrecompiledHeaderStatement.sqlStatement,
                Eq("DELETE FROM precompiledHeaders WHERE projectPartId = (SELECT projectPartId FROM projectParts WHERE projectPartName = ?)"));
}

TEST_F(PrecompiledHeaderStorage, CompilePrecompiledHeaderStatements)
{
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};

    ASSERT_NO_THROW(ClangBackEnd::PrecompiledHeaderStorage<>{database});
}

}
