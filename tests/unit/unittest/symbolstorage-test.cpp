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

using Utils::PathString;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathCachingInterface;
using ClangBackEnd::SymbolEntries;
using ClangBackEnd::SymbolEntry;
using ClangBackEnd::SourceLocationEntries;
using ClangBackEnd::SourceLocationEntry;
using ClangBackEnd::SymbolIndex;
using ClangBackEnd::SourceLocationKind;
using ClangBackEnd::SymbolKind;
using Sqlite::Database;
using Sqlite::Table;

using Storage = ClangBackEnd::SymbolStorage<MockSqliteDatabase>;

class SymbolStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Storage storage{mockDatabase};
    MockSqliteWriteStatement &insertSymbolsToNewSymbolsStatement = storage.m_insertSymbolsToNewSymbolsStatement;
    MockSqliteWriteStatement &insertLocationsToNewLocationsStatement = storage.m_insertLocationsToNewLocationsStatement;
    MockSqliteReadStatement &selectNewSourceIdsStatement = storage.m_selectNewSourceIdsStatement;
    MockSqliteWriteStatement &addNewSymbolsToSymbolsStatement = storage.m_addNewSymbolsToSymbolsStatement;
    MockSqliteWriteStatement &syncNewSymbolsFromSymbolsStatement = storage.m_syncNewSymbolsFromSymbolsStatement;
    MockSqliteWriteStatement &syncSymbolsIntoNewLocationsStatement = storage.m_syncSymbolsIntoNewLocationsStatement;
    MockSqliteWriteStatement &deleteAllLocationsFromUpdatedFilesStatement = storage.m_deleteAllLocationsFromUpdatedFilesStatement;
    MockSqliteWriteStatement &insertNewLocationsInLocationsStatement = storage.m_insertNewLocationsInLocationsStatement;
    MockSqliteWriteStatement &deleteNewSymbolsTableStatement = storage.m_deleteNewSymbolsTableStatement;
    MockSqliteWriteStatement &deleteNewLocationsTableStatement = storage.m_deleteNewLocationsTableStatement;
    MockSqliteWriteStatement &insertProjectPartStatement = storage.m_insertProjectPartStatement;
    MockSqliteWriteStatement &updateProjectPartStatement = storage.m_updateProjectPartStatement;
    MockSqliteReadStatement &getProjectPartIdStatement = storage.m_getProjectPartIdStatement;
    MockSqliteWriteStatement &deleteAllProjectPartsSourcesWithProjectPartIdStatement = storage.m_deleteAllProjectPartsSourcesWithProjectPartIdStatement;
    MockSqliteWriteStatement &insertProjectPartSourcesStatement = storage.m_insertProjectPartSourcesStatement;
    MockSqliteReadStatement &getProjectPartArtefactsBySourceId = storage.m_getProjectPartArtefactsBySourceId;
    MockSqliteReadStatement &getProjectPartArtefactsByProjectPartName = storage.m_getProjectPartArtefactsByProjectPartName;
    MockSqliteReadStatement &getPrecompiledHeader = storage.m_getPrecompiledHeader;

    SymbolEntries symbolEntries{{1, {"functionUSR", "function", SymbolKind::Function}},
                                {2, {"function2USR", "function2", SymbolKind::Function}}};
    SourceLocationEntries sourceLocations{{1, 3, {42, 23}, SourceLocationKind::Declaration},
                                          {2, 4, {7, 11}, SourceLocationKind::Definition}};
    ClangBackEnd::ProjectPartArtefact artefact{"[\"-DFOO\"]", "{\"FOO\":\"1\"}", "[\"/includes\"]", 74};
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

TEST_F(SymbolStorage, ConvertStringsToJson)
{
    Utils::SmallStringVector strings{"foo", "bar", "foo"};

    auto jsonText = storage.toJson(strings);

    ASSERT_THAT(jsonText, Eq("[\"foo\",\"bar\",\"foo\"]"));
}

TEST_F(SymbolStorage, InsertProjectPart)
{
    InSequence sequence;
    ON_CALL(mockDatabase, lastInsertedRowId()).WillByDefault(Return(1));

    EXPECT_CALL(mockDatabase, setLastInsertedRowId(-1));
    EXPECT_CALL(insertProjectPartStatement,
                write(TypedEq<Utils::SmallStringView>("project"),
                      TypedEq<Utils::SmallStringView>("[\"foo\"]"),
                      TypedEq<Utils::SmallStringView>("{\"FOO\":\"1\"}"),
                      TypedEq<Utils::SmallStringView>("[\"/includes\"]")));
    EXPECT_CALL(mockDatabase, lastInsertedRowId()).Times(2);

    storage.insertOrUpdateProjectPart("project",  {"foo"}, {{"FOO", "1"}}, {"/includes"});
}

TEST_F(SymbolStorage, UpdateProjectPart)
{
    InSequence sequence;
    ON_CALL(mockDatabase, lastInsertedRowId()).WillByDefault(Return(-1));

    EXPECT_CALL(mockDatabase, setLastInsertedRowId(-1));
    EXPECT_CALL(insertProjectPartStatement,
                write(TypedEq<Utils::SmallStringView>("project"),
                      TypedEq<Utils::SmallStringView>("[\"foo\"]"),
                      TypedEq<Utils::SmallStringView>("{\"FOO\":\"1\"}"),
                      TypedEq<Utils::SmallStringView>("[\"/includes\"]")));
    EXPECT_CALL(mockDatabase, lastInsertedRowId());
    EXPECT_CALL(updateProjectPartStatement,
                write(TypedEq<Utils::SmallStringView>("[\"foo\"]"),
                      TypedEq<Utils::SmallStringView>("{\"FOO\":\"1\"}"),
                      TypedEq<Utils::SmallStringView>("[\"/includes\"]"),
                      TypedEq<Utils::SmallStringView>("project")));
    EXPECT_CALL(mockDatabase, lastInsertedRowId());

    storage.insertOrUpdateProjectPart("project",  {"foo"}, {{"FOO", "1"}}, {"/includes"});
}

TEST_F(SymbolStorage, UpdateProjectPartSources)
{
    InSequence sequence;

    EXPECT_CALL(deleteAllProjectPartsSourcesWithProjectPartIdStatement, write(TypedEq<int>(42)));
    EXPECT_CALL(insertProjectPartSourcesStatement, write(TypedEq<int>(42), TypedEq<int>(1)));
    EXPECT_CALL(insertProjectPartSourcesStatement, write(TypedEq<int>(42), TypedEq<int>(2)));

    storage.updateProjectPartSources(42, {1, 2});
}

TEST_F(SymbolStorage, FetchProjectPartArtefactBySourceIdCallsValueInStatement)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
            .WillRepeatedly(Return(artefact));

    storage.fetchProjectPartArtefact(1);
}

TEST_F(SymbolStorage, FetchProjectPartArtefactBySourceIdReturnArtefact)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
            .WillRepeatedly(Return(artefact));

    auto result = storage.fetchProjectPartArtefact(1);

    ASSERT_THAT(result, Eq(artefact));
}

TEST_F(SymbolStorage, FetchProjectPartArtefactByProjectNameCallsValueInStatement)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
            .WillRepeatedly(Return(artefact));

    storage.fetchProjectPartArtefact(1);
}

TEST_F(SymbolStorage, FetchProjectPartArtefactByProjectNameReturnArtefact)
{
    EXPECT_CALL(getProjectPartArtefactsBySourceId, valueReturnProjectPartArtefact(1))
            .WillRepeatedly(Return(artefact));

    auto result = storage.fetchProjectPartArtefact(1);

    ASSERT_THAT(result, Eq(artefact));
}

TEST_F(SymbolStorage, FetchPrecompiledHeaderCallsValueInStatement)
{
    EXPECT_CALL(getPrecompiledHeader, valueReturnProjectPartPch(Eq(25)));

    storage.fetchPrecompiledHeader(25);
}

TEST_F(SymbolStorage, FetchPrecompiledHeader)
{
    ClangBackEnd::ProjectPartPch pch{"", "/path/to/pch", 131};
    EXPECT_CALL(getPrecompiledHeader, valueReturnProjectPartPch(Eq(25)))
            .WillRepeatedly(Return(pch));

    auto precompiledHeader = storage.fetchPrecompiledHeader(25);

    ASSERT_THAT(precompiledHeader.value(), Eq(pch));
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

