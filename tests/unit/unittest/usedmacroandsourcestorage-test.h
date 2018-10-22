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

#include <sqlitedatabase.h>
#include <usedmacroandsourcestorage.h>

#include <storagesqlitestatementfactory.h>

#include <utils/optional.h>

namespace {

using Utils::PathString;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathCachingInterface;
using Sqlite::Database;
using Sqlite::Table;

using Storage = ClangBackEnd::UsedMacroAndSourceStorage<MockSqliteDatabase>;

class SymbolStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Storage storage{mockDatabase};
    MockSqliteWriteStatement &insertIntoNewUsedMacrosStatement = storage.m_insertIntoNewUsedMacrosStatement;
    MockSqliteWriteStatement &syncNewUsedMacrosStatement =storage.m_syncNewUsedMacrosStatement;
    MockSqliteWriteStatement &deleteOutdatedUsedMacrosStatement = storage.m_deleteOutdatedUsedMacrosStatement;
    MockSqliteWriteStatement &deleteNewUsedMacrosTableStatement = storage.m_deleteNewUsedMacrosTableStatement;
    MockSqliteWriteStatement &insertFileStatuses = storage.m_insertFileStatuses;
    MockSqliteWriteStatement &insertIntoNewSourceDependenciesStatement = storage.m_insertIntoNewSourceDependenciesStatement;
    MockSqliteWriteStatement &syncNewSourceDependenciesStatement = storage.m_syncNewSourceDependenciesStatement;
    MockSqliteWriteStatement &deleteOutdatedSourceDependenciesStatement = storage.m_deleteOutdatedSourceDependenciesStatement;
    MockSqliteWriteStatement &deleteNewSourceDependenciesStatement = storage.m_deleteNewSourceDependenciesStatement;
    MockSqliteReadStatement &getLowestLastModifiedTimeOfDependencies = storage.m_getLowestLastModifiedTimeOfDependencies;
};

TEST_F(SymbolStorage, ConvertStringsToJson)
{
    Utils::SmallStringVector strings{"foo", "bar", "foo"};

    auto jsonText = storage.toJson(strings);

    ASSERT_THAT(jsonText, Eq("[\"foo\",\"bar\",\"foo\"]"));
}

TEST_F(SymbolStorage, InsertOrUpdateUsedMacros)
{
    InSequence sequence;

    EXPECT_CALL(insertIntoNewUsedMacrosStatement, write(TypedEq<uint>(42u), TypedEq<Utils::SmallStringView>("FOO")));
    EXPECT_CALL(insertIntoNewUsedMacrosStatement, write(TypedEq<uint>(43u), TypedEq<Utils::SmallStringView>("BAR")));
    EXPECT_CALL(syncNewUsedMacrosStatement, execute());
    EXPECT_CALL(deleteOutdatedUsedMacrosStatement, execute());
    EXPECT_CALL(deleteNewUsedMacrosTableStatement, execute());

    storage.insertOrUpdateUsedMacros({{"FOO", {1, 42}}, {"BAR", {1, 43}}});
}

TEST_F(SymbolStorage, InsertFileStatuses)
{
    EXPECT_CALL(insertFileStatuses, write(TypedEq<int>(42), TypedEq<off_t>(1), TypedEq<time_t>(2), TypedEq<bool>(false)));
    EXPECT_CALL(insertFileStatuses, write(TypedEq<int>(43), TypedEq<off_t>(4), TypedEq<time_t>(5), TypedEq<bool>(true)));

    storage.insertFileStatuses({{{1, 42}, 1, 2, false}, {{1, 43}, 4, 5, true}});
}

TEST_F(SymbolStorage, InsertOrUpdateSourceDependencies)
{
    InSequence sequence;

    EXPECT_CALL(insertIntoNewSourceDependenciesStatement, write(TypedEq<int>(42), TypedEq<int>(1)));
    EXPECT_CALL(insertIntoNewSourceDependenciesStatement, write(TypedEq<int>(42), TypedEq<int>(2)));
    EXPECT_CALL(syncNewSourceDependenciesStatement, execute());
    EXPECT_CALL(deleteOutdatedSourceDependenciesStatement, execute());
    EXPECT_CALL(deleteNewSourceDependenciesStatement, execute());

    storage.insertOrUpdateSourceDependencies({{{1, 42}, {1, 1}}, {{1, 42}, {1, 2}}});
}

TEST_F(SymbolStorage, AddTablesInConstructor)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, immediateBegin());
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedMacros(sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedMacros_sourceId_macroName ON newUsedMacros(sourceId, macroName)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSourceDependencies(sourceId INTEGER, dependencySourceId TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSourceDependencies_sourceId_dependencySourceId ON newSourceDependencies(sourceId, dependencySourceId)")));
    EXPECT_CALL(mockDatabase, commit());

    Storage storage{mockDatabase};
}


TEST_F(SymbolStorage, FetchLowestLastModifiedTimeIfNoModificationTimeExists)
{
    EXPECT_CALL(getLowestLastModifiedTimeOfDependencies, valueReturnInt64(Eq(1)));

    auto lowestLastModified = storage.fetchLowestLastModifiedTime({1, 1});

    ASSERT_THAT(lowestLastModified, Eq(0));
}

TEST_F(SymbolStorage, FetchLowestLastModifiedTime)
{
    EXPECT_CALL(getLowestLastModifiedTimeOfDependencies, valueReturnInt64(Eq(21)))
            .WillRepeatedly(Return(12));

    auto lowestLastModified = storage.fetchLowestLastModifiedTime({1, 21});

    ASSERT_THAT(lowestLastModified, Eq(12));
}

TEST_F(SymbolStorage, AddNewUsedMacroTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedMacros(sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedMacros_sourceId_macroName ON newUsedMacros(sourceId, macroName)")));

    factory.createNewUsedMacrosTable();
}

TEST_F(SymbolStorage, AddNewSourceDependenciesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSourceDependencies(sourceId INTEGER, dependencySourceId TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSourceDependencies_sourceId_dependencySourceId ON newSourceDependencies(sourceId, dependencySourceId)")));

    factory.createNewSourceDependenciesTable();
}

}

