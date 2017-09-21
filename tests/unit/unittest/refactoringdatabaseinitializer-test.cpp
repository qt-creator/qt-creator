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
    NiceMock<MockMutex> mockMutex;
    NiceMock<MockSqliteDatabase> mockDatabase{mockMutex};
    Initializer initializer{mockDatabase};
};

TEST_F(RefactoringDatabaseInitializer, AddSymbolsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS symbols(symbolId INTEGER PRIMARY KEY, usr TEXT, symbolName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_usr ON symbols(usr)")));

    initializer.createSymbolsTable();
}

TEST_F(RefactoringDatabaseInitializer, AddLocationsTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS locations(symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_locations_sourceId_line_column ON locations(sourceId, line, column)")));

    initializer.createLocationsTable();
}

TEST_F(RefactoringDatabaseInitializer, AddSourcesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sources(sourceId INTEGER PRIMARY KEY, directoryId INTEGER, sourceName TEXT, sourceType INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_sources_sourceName ON sources(sourceName)")));

    initializer.createSourcesTable();
}

TEST_F(RefactoringDatabaseInitializer, AddDirectoriesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS directories(directoryId INTEGER PRIMARY KEY, directoryPath TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_directories_directoryPath ON directories(directoryPath)")));
    initializer.createDirectoriesTable();
}


TEST_F(RefactoringDatabaseInitializer, CreateInTheContructor)
{
    InSequence s;

    EXPECT_CALL(mockMutex, lock());
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN IMMEDIATE")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS symbols(symbolId INTEGER PRIMARY KEY, usr TEXT, symbolName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_symbols_usr ON symbols(usr)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS locations(symbolId INTEGER, line INTEGER, column INTEGER, sourceId INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_locations_sourceId_line_column ON locations(sourceId, line, column)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS sources(sourceId INTEGER PRIMARY KEY, directoryId INTEGER, sourceName TEXT, sourceType INTEGER)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_sources_sourceName ON sources(sourceName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TABLE IF NOT EXISTS directories(directoryId INTEGER PRIMARY KEY, directoryPath TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_directories_directoryPath ON directories(directoryPath)")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));
    EXPECT_CALL(mockMutex, unlock());

    Initializer initializer{mockDatabase};
}
}
