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

using StatementFactory = ClangBackEnd::StorageSqliteStatementFactory<NiceMock<MockSqliteDatabase>,
                                                                     MockSqliteReadStatement,
                                                                     MockSqliteWriteStatement>;

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

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));

    factory.createNewSymbolsTable();
}

TEST_F(StorageSqliteStatementFactory, AddNewLocationsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newLocations_sourceId ON newLocations(sourceId)")));

    factory.createNewLocationsTable();
}

TEST_F(StorageSqliteStatementFactory, AddNewUsedDefineTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedDefines(sourceId INTEGER, defineName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedDefines_sourceId_defineName ON newUsedDefines(sourceId, defineName)")));

    factory.createNewUsedDefinesTable();
}

TEST_F(StorageSqliteStatementFactory, AddTablesInConstructor)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSymbols(temporarySymbolId INTEGER PRIMARY KEY, symbolId INTEGER, usr TEXT, symbolName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_usr_symbolName ON newSymbols(usr, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSymbols_symbolId ON newSymbols(symbolId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newLocations(temporarySymbolId INTEGER, symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newLocations_sourceId ON newLocations(sourceId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedDefines(sourceId INTEGER, defineName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedDefines_sourceId_defineName ON newUsedDefines(sourceId, defineName)")));
    EXPECT_CALL(mockDatabase, commit());

    StatementFactory factory{mockDatabase};
}

TEST_F(StorageSqliteStatementFactory, InsertNewSymbolsStatement)
{
    ASSERT_THAT(factory.insertSymbolsToNewSymbolsStatement.sqlStatement,
                Eq("INSERT INTO newSymbols(temporarySymbolId, usr, symbolName) VALUES(?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, InsertNewLocationsToLocations)
{
    ASSERT_THAT(factory.insertLocationsToNewLocationsStatement.sqlStatement,
                Eq("INSERT INTO newLocations(temporarySymbolId, line, column, sourceId) VALUES(?,?,?,?)"));
}

TEST_F(StorageSqliteStatementFactory, SelectNewSourceIdsStatement)
{
    ASSERT_THAT(factory.selectNewSourceIdsStatement.sqlStatement,
                Eq("SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources WHERE newLocations.sourceId == sources.sourceId)"));
}

TEST_F(StorageSqliteStatementFactory, AddNewSymbolsToSymbolsStatement)
{
    ASSERT_THAT(factory.addNewSymbolsToSymbolsStatement.sqlStatement,
                Eq("INSERT INTO symbols(usr, symbolName) SELECT usr, symbolName FROM newSymbols WHERE NOT EXISTS (SELECT usr FROM symbols WHERE symbols.usr == newSymbols.usr)"));
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
                Eq("INSERT INTO locations(symbolId, line, column, sourceId) SELECT symbolId, line, column, sourceId FROM newLocations"));
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
                Eq("INSERT OR IGNORE INTO projectParts(projectPartName, compilerArguments) VALUES (?,?)"));
}

TEST_F(StorageSqliteStatementFactory, UpdateProjectPart)
{
    ASSERT_THAT(factory.updateProjectPartStatement.sqlStatement,
                Eq("UPDATE projectParts SET compilerArguments = ? WHERE projectPartName = ?"));
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

TEST_F(StorageSqliteStatementFactory, InsertIntoNewUsedDefines)
{
    ASSERT_THAT(factory.insertIntoNewUsedDefinesStatement.sqlStatement,
                Eq("INSERT INTO newUsedDefines(sourceId, defineName) VALUES (?,?)"));
}

TEST_F(StorageSqliteStatementFactory, SyncNewUsedDefines)
{
    ASSERT_THAT(factory.syncNewUsedDefinesStatement.sqlStatement,
                Eq("INSERT INTO usedDefines(sourceId, defineName) SELECT sourceId, defineName FROM newUsedDefines WHERE NOT EXISTS (SELECT sourceId FROM usedDefines WHERE usedDefines.sourceId == newUsedDefines.sourceId AND usedDefines.defineName == newUsedDefines.defineName)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteUnusedDefines)
{
    ASSERT_THAT(factory.deleteOutdatedUsedDefinesStatement.sqlStatement,
                Eq("DELETE FROM usedDefines WHERE sourceId IN (SELECT sourceId FROM newUsedDefines) AND NOT EXISTS (SELECT sourceId FROM newUsedDefines WHERE newUsedDefines.sourceId == usedDefines.sourceId AND newUsedDefines.defineName == usedDefines.defineName)"));
}

TEST_F(StorageSqliteStatementFactory, DeleteAllInNewUnusedDefines)
{
    ASSERT_THAT(factory.deleteNewUsedDefinesTableStatement.sqlStatement,
                Eq("DELETE FROM newUsedDefines"));
}
}
