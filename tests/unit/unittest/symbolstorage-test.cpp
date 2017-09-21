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

#include "mockfilepathcaching.h"
#include "mocksqlitereadstatement.h"
#include "mocksqlitewritestatement.h"

#include <storagesqlitestatementfactory.h>
#include <symbolstorage.h>
#include <sqlitedatabase.h>

#include <storagesqlitestatementfactory.h>

namespace {

using Utils::PathString;
using ClangBackEnd::FilePathCachingInterface;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::StorageSqliteStatementFactory;
using ClangBackEnd::SymbolType;
using Sqlite::Database;
using Sqlite::Table;

using StatementFactory = StorageSqliteStatementFactory<MockSqliteDatabase,
                                                MockSqliteReadStatement,
                                                MockSqliteWriteStatement>;
using Storage = ClangBackEnd::SymbolStorage<StatementFactory>;

class SymbolStorage : public testing::Test
{
protected:
    void SetUp();

protected:
    MockFilePathCaching filePathCache;
    NiceMock<MockMutex> mockMutex;
    NiceMock<MockSqliteDatabase> mockDatabase{mockMutex};
    StatementFactory statementFactory{mockDatabase};
    MockSqliteWriteStatement &insertSymbolsToNewSymbolsStatement = statementFactory.insertSymbolsToNewSymbolsStatement;
    MockSqliteWriteStatement &insertLocationsToNewLocationsStatement = statementFactory.insertLocationsToNewLocationsStatement;
    MockSqliteReadStatement &selectNewSourceIdsStatement = statementFactory.selectNewSourceIdsStatement;
    MockSqliteWriteStatement &addNewSymbolsToSymbolsStatement = statementFactory.addNewSymbolsToSymbolsStatement;
    MockSqliteWriteStatement &syncNewSymbolsFromSymbolsStatement = statementFactory.syncNewSymbolsFromSymbolsStatement;
    MockSqliteWriteStatement &syncSymbolsIntoNewLocationsStatement = statementFactory.syncSymbolsIntoNewLocationsStatement;
    MockSqliteWriteStatement &deleteAllLocationsFromUpdatedFilesStatement = statementFactory.deleteAllLocationsFromUpdatedFilesStatement;
    MockSqliteWriteStatement &insertNewLocationsInLocationsStatement = statementFactory.insertNewLocationsInLocationsStatement;
    MockSqliteWriteStatement &deleteNewSymbolsTableStatement = statementFactory.deleteNewSymbolsTableStatement;
    MockSqliteWriteStatement &deleteNewLocationsTableStatement = statementFactory.deleteNewLocationsTableStatement;
    SymbolEntries symbolEntries{{1, {"functionUSR", "function"}},
                                {2, {"function2USR", "function2"}}};
    SourceLocationEntries sourceLocations{{1, {1, 3}, {42, 23}, SymbolType::Declaration},
                                          {2, {1, 4}, {7, 11}, SymbolType::Declaration}};
    Storage storage{statementFactory, filePathCache};
};

TEST_F(SymbolStorage, CreateAndFillTemporaryLocationsTable)
{
    InSequence sequence;

    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(1, 42, 23, 3));
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(2, 7, 11, 4));

    storage.fillTemporaryLocationsTable(sourceLocations);
}

TEST_F(SymbolStorage, AddNewSymbolsToSymbols)
{
    EXPECT_CALL(addNewSymbolsToSymbolsStatement, execute());

    storage.addNewSymbolsToSymbols();
}

TEST_F(SymbolStorage, SyncNewSymbolsFromSymbols)
{
    EXPECT_CALL(syncNewSymbolsFromSymbolsStatement, execute());

    storage.syncNewSymbolsFromSymbols();
}

TEST_F(SymbolStorage, SyncSymbolsIntoNewLocations)
{
    EXPECT_CALL(syncSymbolsIntoNewLocationsStatement, execute());

    storage.syncSymbolsIntoNewLocations();
}

TEST_F(SymbolStorage, DeleteAllLocationsFromUpdatedFiles)
{
    EXPECT_CALL(deleteAllLocationsFromUpdatedFilesStatement, execute());

    storage.deleteAllLocationsFromUpdatedFiles();
}

TEST_F(SymbolStorage, InsertNewLocationsInLocations)
{
    EXPECT_CALL(insertNewLocationsInLocationsStatement, execute());

    storage.insertNewLocationsInLocations();
}

TEST_F(SymbolStorage, DropNewSymbolsTable)
{
    EXPECT_CALL(deleteNewSymbolsTableStatement, execute());

    storage.deleteNewSymbolsTable();
}

TEST_F(SymbolStorage, DropNewLocationsTable)
{
    EXPECT_CALL(deleteNewLocationsTableStatement, execute());

    storage.deleteNewLocationsTable();
}

TEST_F(SymbolStorage, AddSymbolsAndSourceLocationsCallsWrite)
{
    InSequence sequence;

    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN IMMEDIATE")));
    EXPECT_CALL(insertSymbolsToNewSymbolsStatement, write(_, _, _)).Times(2);
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(1, 42, 23, 3));
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(2, 7, 11, 4));
    EXPECT_CALL(addNewSymbolsToSymbolsStatement, execute());
    EXPECT_CALL(syncNewSymbolsFromSymbolsStatement, execute());
    EXPECT_CALL(syncSymbolsIntoNewLocationsStatement, execute());
    EXPECT_CALL(deleteAllLocationsFromUpdatedFilesStatement, execute());
    EXPECT_CALL(insertNewLocationsInLocationsStatement, execute());
    EXPECT_CALL(deleteNewSymbolsTableStatement, execute());
    EXPECT_CALL(deleteNewLocationsTableStatement, execute());
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));
    EXPECT_CALL(mockMutex, unlock());

    storage.addSymbolsAndSourceLocations(symbolEntries, sourceLocations);
}

void SymbolStorage::SetUp()
{
}
}

