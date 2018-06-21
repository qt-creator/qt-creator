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
#include "mocksqlitereadstatement.h"
#include "mocksqlitewritestatement.h"

#include <storagesqlitestatementfactory.h>

namespace {

using StatementFactory = ClangBackEnd::StorageSqliteStatementFactory<NiceMock<MockSqliteDatabase>>;

using Sqlite::Table;

class StorageSqliteStatementFactory : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    StatementFactory factory{mockDatabase};
};

TEST_F(StorageSqliteStatementFactory, AddNewSymbolsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT, symbolKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));

    factory.createNewSymbolsTable();
}

TEST_F(StorageSqliteStatementFactory, AddNewLocationsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, sourceId INTEGER, line INTEGER, column INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_newLocations_sourceId_line_column ON newLocations(sourceId, line, column)")));

    factory.createNewLocationsTable();
}

TEST_F(StorageSqliteStatementFactory, AddNewUsedMacroTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedMacros(sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedMacros_sourceId_macroName ON newUsedMacros(sourceId, macroName)")));

    factory.createNewUsedMacrosTable();
}

TEST_F(StorageSqliteStatementFactory, AddNewSourceDependenciesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSourceDependencies(sourceId INTEGER, dependencySourceId TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSourceDependencies_sourceId_dependencySourceId ON newSourceDependencies(sourceId, dependencySourceId)")));

    factory.createNewSourceDependenciesTable();
}

TEST_F(StorageSqliteStatementFactory, AddTablesInConstructor)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT, symbolKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, sourceId INTEGER, line INTEGER, column INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_newLocations_sourceId_line_column ON newLocations(sourceId, line, column)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedMacros(sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedMacros_sourceId_macroName ON newUsedMacros(sourceId, macroName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSourceDependencies(sourceId INTEGER, dependencySourceId TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSourceDependencies_sourceId_dependencySourceId ON newSourceDependencies(sourceId, dependencySourceId)")));
    EXPECT_CALL(mockDatabase, commit());

    StatementFactory factory{mockDatabase};
}

TEST_F(StorageSqliteStatementFactory, InsertNewSymbolsStatement)
{
    ASSERT_THAT(factory.insertSymbolsToNewSymbolsStatement.sqlStatement,
                Eq("INSERT INTO newSymbols(temporarySymbolId, usr, symbolName, symbolKind) VALUES(?,?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, InsertNewLocationsToLocations)
{
    ASSERT_THAT(factory.insertLocationsToNewLocationsStatement.sqlStatement,
                Eq("INSERT OR IGNORE INTO newLocations(temporarySymbolId, line, column, sourceId, locationKind) VALUES(?,?,?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, SelectNewSourceIdsStatement)
{
    ASSERT_THAT(factory.selectNewSourceIdsStatement.sqlStatement,
                Eq("SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources WHERE newLocations.sourceId == sources.sourceId)"));
}

TEST_F(StorageSqliteStatementFactory, AddNewSymbolsToSymbolsStatement)
{
    ASSERT_THAT(factory.addNewSymbolsToSymbolsStatement.sqlStatement,
                Eq("INSERT INTO symbols(usr, symbolName, symbolKind) SELECT usr, symbolName, symbolKind FROM newSymbols WHERE NOT EXISTS (SELECT usr FROM symbols WHERE symbols.usr == newSymbols.usr)"));
}

TEST_F(StorageSqliteStatementFactory, SyncNewSymbolsFromSymbolsStatement)
{
    ASSERT_THAT(factory.syncNewSymbolsFromSymbolsStatement.sqlStatement,
                Eq("UPDATE newSymbols SET symbolId = (SELECT symbolId FROM symbols WHERE newSymbols.usr = symbols.usr)"));
}

TEST_F(StorageSqliteStatementFactory, SyncSymbolsIntoNewLocations)
{
    ASSERT_THAT(factory.syncSymbolsIntoNewLocationsStatement.sqlStatement,
                Eq("UPDATE newLocations SET symbolId = (SELECT symbolId FROM newSymbols WHERE newSymbols.temporarySymbolId = newLocations.temporarySymbolId)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteAllLocationsFromUpdatedFiles)
{
    ASSERT_THAT(factory.deleteAllLocationsFromUpdatedFilesStatement.sqlStatement,
                Eq("DELETE FROM locations WHERE sourceId IN (SELECT DISTINCT sourceId FROM newLocations)"));
}

TEST_F(StorageSqliteStatementFactory, InsertNewLocationsInLocations)
{
    ASSERT_THAT(factory.insertNewLocationsInLocationsStatement.sqlStatement,
                Eq("INSERT INTO locations(symbolId, line, column, sourceId, locationKind) SELECT symbolId, line, column, sourceId, locationKind FROM newLocations"));
}

TEST_F(StorageSqliteStatementFactory, DeleteNewSymbolsTableStatement)
{
    ASSERT_THAT(factory.deleteNewSymbolsTableStatement.sqlStatement,
                Eq("DELETE FROM newSymbols"));
}

TEST_F(StorageSqliteStatementFactory, DeleteNewLocationsTableStatement)
{
    ASSERT_THAT(factory.deleteNewLocationsTableStatement.sqlStatement,
                Eq("DELETE FROM newLocations"));
}

TEST_F(StorageSqliteStatementFactory, InsertProjectPart)
{
    ASSERT_THAT(factory.insertProjectPartStatement.sqlStatement,
                Eq("INSERT OR IGNORE INTO projectParts(projectPartName, compilerArguments, compilerMacros, includeSearchPaths) VALUES (?,?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, UpdateProjectPart)
{
    ASSERT_THAT(factory.updateProjectPartStatement.sqlStatement,
                Eq("UPDATE projectParts SET compilerArguments = ?, compilerMacros = ?, includeSearchPaths = ? WHERE projectPartName = ?"));
}

TEST_F(StorageSqliteStatementFactory, GetProjectPartIdForProjectPartName)
{
    ASSERT_THAT(factory.getProjectPartIdStatement.sqlStatement,
                Eq("SELECT projectPartId FROM projectParts WHERE projectPartName = ?"));
}

TEST_F(StorageSqliteStatementFactory, DeleteAllProjectPartsSourcesWithProjectPartId)
{
    ASSERT_THAT(factory.deleteAllProjectPartsSourcesWithProjectPartIdStatement.sqlStatement,
                Eq("DELETE FROM projectPartsSources WHERE projectPartId = ?"));
}

TEST_F(StorageSqliteStatementFactory, InsertProjectPartsSources)
{
    ASSERT_THAT(factory.insertProjectPartSourcesStatement.sqlStatement,
                Eq("INSERT INTO projectPartsSources(projectPartId, sourceId) VALUES (?,?)"));
}

TEST_F(StorageSqliteStatementFactory, GetCompileArgumentsForFileId)
{
    ASSERT_THAT(factory.getCompileArgumentsForFileIdStatement.sqlStatement,
                Eq("SELECT compilerArguments FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)"));
}

TEST_F(StorageSqliteStatementFactory, InsertIntoNewUsedMacros)
{
    ASSERT_THAT(factory.insertIntoNewUsedMacrosStatement.sqlStatement,
                Eq("INSERT INTO newUsedMacros(sourceId, macroName) VALUES (?,?)"));
}

TEST_F(StorageSqliteStatementFactory, SyncNewUsedMacros)
{
    ASSERT_THAT(factory.syncNewUsedMacrosStatement.sqlStatement,
                Eq("INSERT INTO usedMacros(sourceId, macroName) SELECT sourceId, macroName FROM newUsedMacros WHERE NOT EXISTS (SELECT sourceId FROM usedMacros WHERE usedMacros.sourceId == newUsedMacros.sourceId AND usedMacros.macroName == newUsedMacros.macroName)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteOutdatedUnusedMacros)
{
    ASSERT_THAT(factory.deleteOutdatedUsedMacrosStatement.sqlStatement,
                Eq("DELETE FROM usedMacros WHERE sourceId IN (SELECT sourceId FROM newUsedMacros) AND NOT EXISTS (SELECT sourceId FROM newUsedMacros WHERE newUsedMacros.sourceId == usedMacros.sourceId AND newUsedMacros.macroName == usedMacros.macroName)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteAllInNewUnusedMacros)
{
    ASSERT_THAT(factory.deleteNewUsedMacrosTableStatement.sqlStatement,
                Eq("DELETE FROM newUsedMacros"));
}

TEST_F(StorageSqliteStatementFactory, InsertFileStatuses)
{
    ASSERT_THAT(factory.insertFileStatuses.sqlStatement,
                Eq("INSERT OR REPLACE INTO fileStatuses(sourceId, size, lastModified, isInPrecompiledHeader) VALUES (?,?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, InsertIntoNewSourceDependencies)
{
    ASSERT_THAT(factory.insertIntoNewSourceDependenciesStatement.sqlStatement,
                Eq("INSERT INTO newSourceDependencies(sourceId, dependencySourceId) VALUES (?,?)"));
}

TEST_F(StorageSqliteStatementFactory, SyncNewSourceDependencies)
{
    ASSERT_THAT(factory.syncNewSourceDependenciesStatement.sqlStatement,
                Eq("INSERT INTO sourceDependencies(sourceId, dependencySourceId) SELECT sourceId, dependencySourceId FROM newSourceDependencies WHERE NOT EXISTS (SELECT sourceId FROM sourceDependencies WHERE sourceDependencies.sourceId == newSourceDependencies.sourceId AND sourceDependencies.dependencySourceId == newSourceDependencies.dependencySourceId)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteOutdatedSourceDependencies)
{
    ASSERT_THAT(factory.deleteOutdatedSourceDependenciesStatement.sqlStatement,
                Eq("DELETE FROM sourceDependencies WHERE sourceId IN (SELECT sourceId FROM newSourceDependencies) AND NOT EXISTS (SELECT sourceId FROM newSourceDependencies WHERE newSourceDependencies.sourceId == sourceDependencies.sourceId AND newSourceDependencies.dependencySourceId == sourceDependencies.dependencySourceId)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteAllInNewSourceDependencies)
{
    ASSERT_THAT(factory.deleteNewSourceDependenciesStatement.sqlStatement,
                Eq("DELETE FROM newSourceDependencies"));
}

TEST_F(StorageSqliteStatementFactory, GetProjectPartCompilerArgumentsAndCompilerMacrosBySourceId)
{
    ASSERT_THAT(factory.getProjectPartArtefactsBySourceId.sqlStatement,
                Eq("SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)"));
}

TEST_F(StorageSqliteStatementFactory, GetProjectPartArtefactsByProjectPartName)
{
    ASSERT_THAT(factory.getProjectPartArtefactsByProjectPartName.sqlStatement,
                Eq("SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartName = ?"));

}

TEST_F(StorageSqliteStatementFactory, GetLowestLastModifiedTimeOfDependencies)
{
    ASSERT_THAT(factory.getLowestLastModifiedTimeOfDependencies.sqlStatement,
                Eq("WITH RECURSIVE sourceIds(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, sourceIds WHERE sourceDependencies.sourceId = sourceIds.sourceId) SELECT min(lastModified) FROM fileStatuses, sourceIds WHERE fileStatuses.sourceId = sourceIds.sourceId"));
}

TEST_F(StorageSqliteStatementFactory, GetPrecompiledHeaderForProjectPartName)
{
    ASSERT_THAT(factory.getPrecompiledHeader.sqlStatement,
                Eq("SELECT pchPath, pchBuildTime FROM precompiledHeaders WHERE projectPartId = ?"));
}

}
