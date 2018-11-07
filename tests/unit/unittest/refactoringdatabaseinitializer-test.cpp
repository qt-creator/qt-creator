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

#include <mocksqlitedatabase.h>

#include <refactoringdatabaseinitializer.h>

namespace {

using Initializer = ClangBackEnd::RefactoringDatabaseInitializer<NiceMock<MockSqliteDatabase>>;

using Sqlite::Table;

class RefactoringDatabaseInitializer : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Initializer initializer{mockDatabase};
};

TEST_F(RefactoringDatabaseInitializer, AddSymbolsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS symbols(symbolId INTEGER PRIMARY KEY, usr TEXT, symbolName TEXT, symbolKind INTEGER, signature TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_usr ON symbols(usr)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_symbolKind_symbolName ON symbols(symbolKind, symbolName)")));

    initializer.createSymbolsTable();
}

TEST_F(RefactoringDatabaseInitializer, AddLocationsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS locations(symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_locations_sourceId_line_column ON locations(sourceId, line, column)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_locations_sourceId_locationKind ON locations(sourceId, locationKind)")));

    initializer.createLocationsTable();
}

TEST_F(RefactoringDatabaseInitializer, AddSourcesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sources(sourceId INTEGER PRIMARY KEY, directoryId INTEGER, sourceName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_sources_directoryId_sourceName ON sources(directoryId, sourceName)")));

    initializer.createSourcesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddDirectoriesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS directories(directoryId INTEGER PRIMARY KEY, directoryPath TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_directories_directoryPath ON directories(directoryPath)")));

    initializer.createDirectoriesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddProjectPartsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS projectParts(projectPartId INTEGER PRIMARY KEY, projectPartName TEXT, compilerArguments TEXT, compilerMacros TEXT, includeSearchPaths TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_projectParts_projectPartName ON projectParts(projectPartName)")));

    initializer.createProjectPartsTable();
}

TEST_F(RefactoringDatabaseInitializer, AddProjectPartsSourcesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS projectPartsSources(projectPartId INTEGER, sourceId INTEGER, sourceType INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_projectPartsSources_sourceId_projectPartId ON projectPartsSources(sourceId, projectPartId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_projectPartsSources_projectPartId ON projectPartsSources(projectPartId)")));

    initializer.createProjectPartsSourcesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddUsedMacrosTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS usedMacros(usedMacroId INTEGER PRIMARY KEY, sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_usedMacros_sourceId_macroName ON usedMacros(sourceId, macroName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_usedMacros_macroName ON usedMacros(macroName)")));

    initializer.createUsedMacrosTable();
}

TEST_F(RefactoringDatabaseInitializer, AddFileStatusesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS fileStatuses(sourceId INTEGER PRIMARY KEY, size INTEGER, lastModified INTEGER, buildDependencyTimeStamp INTEGER, isInPrecompiledHeader INTEGER)")));

    initializer.createFileStatusesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddSourceDependenciesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sourceDependencies(sourceId INTEGER, dependencySourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_sourceDependencies_sourceId_dependencySourceId ON sourceDependencies(sourceId, dependencySourceId)")));

    initializer.createSourceDependenciesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddPrecompiledHeaderTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS precompiledHeaders(projectPartId INTEGER PRIMARY KEY, pchPath TEXT, pchBuildTime INTEGER)")));

    initializer.createPrecompiledHeadersTable();
}

TEST_F(RefactoringDatabaseInitializer, CreateInTheContructor)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS symbols(symbolId INTEGER PRIMARY KEY, usr TEXT, symbolName TEXT, symbolKind INTEGER, signature TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_usr ON symbols(usr)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_symbolKind_symbolName ON symbols(symbolKind, symbolName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS locations(symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER, locationKind INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_locations_sourceId_line_column ON locations(sourceId, line, column)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_locations_sourceId_locationKind ON locations(sourceId, locationKind)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sources(sourceId INTEGER PRIMARY KEY, directoryId INTEGER, sourceName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_sources_directoryId_sourceName ON sources(directoryId, sourceName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS directories(directoryId INTEGER PRIMARY KEY, directoryPath TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_directories_directoryPath ON directories(directoryPath)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS projectParts(projectPartId INTEGER PRIMARY KEY, projectPartName TEXT, compilerArguments TEXT, compilerMacros TEXT, includeSearchPaths TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_projectParts_projectPartName ON projectParts(projectPartName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS projectPartsSources(projectPartId INTEGER, sourceId INTEGER, sourceType INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE UNIQUE INDEX IF NOT EXISTS index_projectPartsSources_sourceId_projectPartId ON projectPartsSources(sourceId, projectPartId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_projectPartsSources_projectPartId ON projectPartsSources(projectPartId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS usedMacros(usedMacroId INTEGER PRIMARY KEY, sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_usedMacros_sourceId_macroName ON usedMacros(sourceId, macroName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_usedMacros_macroName ON usedMacros(macroName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS fileStatuses(sourceId INTEGER PRIMARY KEY, size INTEGER, lastModified INTEGER, buildDependencyTimeStamp INTEGER, isInPrecompiledHeader INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sourceDependencies(sourceId INTEGER, dependencySourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_sourceDependencies_sourceId_dependencySourceId ON sourceDependencies(sourceId, dependencySourceId)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS precompiledHeaders(projectPartId INTEGER PRIMARY KEY, pchPath TEXT, pchBuildTime INTEGER)")));
    EXPECT_CALL(mockDatabase, commit());

    Initializer initializer{mockDatabase};
}
}
