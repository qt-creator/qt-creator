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

#include <builddependenciesstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>
#include <symbolstorage.h>

#include <utils/optional.h>

namespace {
using ClangBackEnd::FilePathCachingInterface;
using ClangBackEnd::FilePathId;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SourceTimeStamp;
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
    MockSqliteWriteStatement &inserOrUpdateIndexingTimesStampStatement = storage.inserOrUpdateIndexingTimesStampStatement;
    MockSqliteReadStatement &fetchIndexingTimeStampsStatement = storage.fetchIndexingTimeStampsStatement;
    MockSqliteReadStatement &fetchIncludedIndexingTimeStampsStatement = storage.fetchIncludedIndexingTimeStampsStatement;
    MockSqliteReadStatement &fetchDependentSourceIdsStatement = storage.fetchDependentSourceIdsStatement;
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

TEST_F(SymbolStorage, FetchIndexingTimeStampsIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchIndexingTimeStampsStatement, valuesReturnSourceTimeStamps(1024))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchIndexingTimeStampsStatement, valuesReturnSourceTimeStamps(1024));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchIndexingTimeStamps();
}

TEST_F(SymbolStorage, InsertIndexingTimeStamp)
{
    ClangBackEnd::FileStatuses fileStatuses{{1, 0, 34}, {2, 0, 37}};

    EXPECT_CALL(inserOrUpdateIndexingTimesStampStatement, write(TypedEq<int>(1), TypedEq<int>(34)));
    EXPECT_CALL(inserOrUpdateIndexingTimesStampStatement, write(TypedEq<int>(2), TypedEq<int>(37)));

    storage.insertOrUpdateIndexingTimeStamps(fileStatuses);
}

TEST_F(SymbolStorage, InsertIndexingTimeStampsIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(inserOrUpdateIndexingTimesStampStatement, write(TypedEq<int>(1), TypedEq<int>(34)));
    EXPECT_CALL(inserOrUpdateIndexingTimesStampStatement, write(TypedEq<int>(2), TypedEq<int>(34)));
    EXPECT_CALL(mockDatabase, commit());

    storage.insertOrUpdateIndexingTimeStamps({1, 2}, 34);
}

TEST_F(SymbolStorage, FetchIncludedIndexingTimeStampsIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchIncludedIndexingTimeStampsStatement,
                valuesReturnSourceTimeStamps(1024, TypedEq<int>(1)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchIncludedIndexingTimeStampsStatement,
                valuesReturnSourceTimeStamps(1024, TypedEq<int>(1)));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchIncludedIndexingTimeStamps(1);
}

TEST_F(SymbolStorage, FetchDependentSourceIdsIsBusy)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchDependentSourceIdsStatement, valuesReturnFilePathIds(1024, TypedEq<int>(3)));
    EXPECT_CALL(fetchDependentSourceIdsStatement, valuesReturnFilePathIds(1024, TypedEq<int>(2)))
        .WillOnce(Throw(Sqlite::StatementIsBusy{""}));
    EXPECT_CALL(mockDatabase, rollback());
    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(fetchDependentSourceIdsStatement, valuesReturnFilePathIds(1024, TypedEq<int>(3)));
    EXPECT_CALL(fetchDependentSourceIdsStatement, valuesReturnFilePathIds(1024, TypedEq<int>(2)));
    EXPECT_CALL(fetchDependentSourceIdsStatement, valuesReturnFilePathIds(1024, TypedEq<int>(7)));
    EXPECT_CALL(mockDatabase, commit());

    storage.fetchDependentSourceIds({3, 2, 7});
}

class SymbolStorageSlow : public testing::Test
{
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::SymbolStorage<> storage{database};
    ClangBackEnd::BuildDependenciesStorage<> buildDependenciesStorage{database};
};

TEST_F(SymbolStorageSlow, InsertIndexingTimeStamps)
{
    storage.insertOrUpdateIndexingTimeStamps({1, 2}, 34);

    ASSERT_THAT(storage.fetchIndexingTimeStamps(),
                ElementsAre(SourceTimeStamp{1, 34}, SourceTimeStamp{2, 34}));
}

TEST_F(SymbolStorageSlow, UpdateIndexingTimeStamps)
{
    storage.insertOrUpdateIndexingTimeStamps({1, 2}, 34);

    storage.insertOrUpdateIndexingTimeStamps({1}, 37);

    ASSERT_THAT(storage.fetchIndexingTimeStamps(),
                ElementsAre(SourceTimeStamp{1, 37}, SourceTimeStamp{2, 34}));
}

TEST_F(SymbolStorageSlow, InsertIndexingTimeStamp)
{
    storage.insertOrUpdateIndexingTimeStamps({{1, 0, 34}, {2, 0, 37}});

    ASSERT_THAT(storage.fetchIndexingTimeStamps(),
                ElementsAre(SourceTimeStamp{1, 34}, SourceTimeStamp{2, 37}));
}

TEST_F(SymbolStorageSlow, UpdateIndexingTimeStamp)
{
    storage.insertOrUpdateIndexingTimeStamps({{1, 0, 34}, {2, 0, 34}});

    storage.insertOrUpdateIndexingTimeStamps({{2, 0, 37}});

    ASSERT_THAT(storage.fetchIndexingTimeStamps(),
                ElementsAre(SourceTimeStamp{1, 34}, SourceTimeStamp{2, 37}));
}

TEST_F(SymbolStorageSlow, FetchIncludedIndexingTimeStamps)
{
    storage.insertOrUpdateIndexingTimeStamps({1, 2, 3, 4, 5}, 34);
    buildDependenciesStorage.insertOrUpdateSourceDependencies({{1, 2}, {1, 3}, {2, 3}, {3, 4}, {5, 3}});

    auto timeStamps = storage.fetchIncludedIndexingTimeStamps(1);

    ASSERT_THAT(timeStamps,
                ElementsAre(SourceTimeStamp{1, 34},
                            SourceTimeStamp{2, 34},
                            SourceTimeStamp{3, 34},
                            SourceTimeStamp{4, 34}));
}

TEST_F(SymbolStorageSlow, FetchDependentSourceIds)
{
    buildDependenciesStorage.insertOrUpdateSourceDependencies(
        {{1, 2}, {1, 3}, {2, 3}, {4, 2}, {5, 6}, {7, 6}});

    auto sourceIds = storage.fetchDependentSourceIds({3, 2, 7});

    ASSERT_THAT(sourceIds, ElementsAre(FilePathId{1}, FilePathId{4}, FilePathId{7}));
}
} // namespace
