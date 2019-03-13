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
#include "mocksqlitedatabase.h"

#include <symbolstorage.h>
#include <sqlitedatabase.h>

#include <utils/optional.h>

namespace {

using ClangBackEnd::FilePathCachingInterface;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SymbolIndex;
using ClangBackEnd::SymbolKind;
using Sqlite::Database;
using Sqlite::Table;
using Utils::PathString;

using Storage = ClangBackEnd::SymbolStorage<MockSqliteDatabase>;

class SymbolStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Storage storage{mockDatabase};
    MockSqliteWriteStatement &insertSymbolsToNewSymbolsStatement = storage.insertSymbolsToNewSymbolsStatement;
    MockSqliteWriteStatement &insertLocationsToNewLocationsStatement = storage.insertLocationsToNewLocationsStatement;
    MockSqliteReadStatement &selectNewSourceIdsStatement = storage.selectNewSourceIdsStatement;
    MockSqliteWriteStatement &addNewSymbolsToSymbolsStatement = storage.addNewSymbolsToSymbolsStatement;
    MockSqliteWriteStatement &syncNewSymbolsFromSymbolsStatement = storage.syncNewSymbolsFromSymbolsStatement;
    MockSqliteWriteStatement &syncSymbolsIntoNewLocationsStatement = storage.syncSymbolsIntoNewLocationsStatement;
    MockSqliteWriteStatement &deleteAllLocationsFromUpdatedFilesStatement = storage.deleteAllLocationsFromUpdatedFilesStatement;
    MockSqliteWriteStatement &insertNewLocationsInLocationsStatement = storage.insertNewLocationsInLocationsStatement;
    MockSqliteWriteStatement &deleteNewSymbolsTableStatement = storage.deleteNewSymbolsTableStatement;
    MockSqliteWriteStatement &deleteNewLocationsTableStatement = storage.deleteNewLocationsTableStatement;
    SymbolEntries symbolEntries{{1, {"functionUSR", "function", SymbolKind::Function}},
                                {2, {"function2USR", "function2", SymbolKind::Function}}};
    SourceLocationEntries sourceLocations{{1, 3, {42, 23}, SourceLocationKind::Declaration},
                                          {2, 4, {7, 11}, SourceLocationKind::Definition}};
};

TEST_F(SymbolStorage, CreateAndFillTemporaryLocationsTable)
{
    InSequence sequence;

    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(TypedEq<SymbolIndex>(1), TypedEq<int>(42), TypedEq<int>(23), TypedEq<int>(3), TypedEq<int>(int(SourceLocationKind::Declaration))));
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(TypedEq<SymbolIndex>(2), TypedEq<int>(7), TypedEq<int>(11), TypedEq<int>(4), TypedEq<int>(int(SourceLocationKind::Definition))));

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

    EXPECT_CALL(insertSymbolsToNewSymbolsStatement, write(An<uint>(), An<Utils::SmallStringView>(), An<Utils::SmallStringView>(), An<uint>())).Times(2);
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(TypedEq<SymbolIndex>(1), TypedEq<int>(42), TypedEq<int>(23), TypedEq<int>(3), TypedEq<int>(int(SourceLocationKind::Declaration))));
    EXPECT_CALL(insertLocationsToNewLocationsStatement, write(TypedEq<SymbolIndex>(2), TypedEq<int>(7), TypedEq<int>(11), TypedEq<int>(4), TypedEq<int>(int(SourceLocationKind::Definition))));
    EXPECT_CALL(addNewSymbolsToSymbolsStatement, execute());
    EXPECT_CALL(syncNewSymbolsFromSymbolsStatement, execute());
    EXPECT_CALL(syncSymbolsIntoNewLocationsStatement, execute());
    EXPECT_CALL(deleteAllLocationsFromUpdatedFilesStatement, execute());
    EXPECT_CALL(insertNewLocationsInLocationsStatement, execute());
    EXPECT_CALL(deleteNewSymbolsTableStatement, execute());
    EXPECT_CALL(deleteNewLocationsTableStatement, execute());

    storage.addSymbolsAndSourceLocations(symbolEntries, sourceLocations);
}

TEST_F(SymbolStorage, AddNewSymbolsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT, symbolKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));

    storage.createNewSymbolsTable();
}

TEST_F(SymbolStorage, AddNewLocationsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, sourceId INTEGER, line INTEGER, column INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_newLocations_sourceId_line_column ON newLocations(sourceId, line, column)")));

    storage.createNewLocationsTable();
}

TEST_F(SymbolStorage, AddTablesInConstructor)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT, symbolKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, sourceId INTEGER, line INTEGER, column INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_newLocations_sourceId_line_column ON newLocations(sourceId, line, column)")));
    EXPECT_CALL(mockDatabase, commit());

    Storage storage{mockDatabase};
}

}

