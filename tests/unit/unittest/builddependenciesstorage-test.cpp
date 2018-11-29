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
#include <builddependenciesstorage.h>

#include <utils/optional.h>

namespace {

using Utils::PathString;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathCachingInterface;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceType;
using ClangBackEnd::UsedMacro;
using Sqlite::Database;
using Sqlite::Table;

using Storage = ClangBackEnd::BuildDependenciesStorage<MockSqliteDatabase>;

class BuildDependenciesStorage : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    Storage storage{mockDatabase};
    MockSqliteWriteStatement &insertIntoNewUsedMacrosStatement = storage.insertIntoNewUsedMacrosStatement;
    MockSqliteWriteStatement &syncNewUsedMacrosStatement =storage.syncNewUsedMacrosStatement;
    MockSqliteWriteStatement &deleteOutdatedUsedMacrosStatement = storage.deleteOutdatedUsedMacrosStatement;
    MockSqliteWriteStatement &deleteNewUsedMacrosTableStatement = storage.deleteNewUsedMacrosTableStatement;
    MockSqliteWriteStatement &insertFileStatuses = storage.insertFileStatusesStatement;
    MockSqliteWriteStatement &insertIntoNewSourceDependenciesStatement = storage.insertIntoNewSourceDependenciesStatement;
    MockSqliteWriteStatement &syncNewSourceDependenciesStatement = storage.syncNewSourceDependenciesStatement;
    MockSqliteWriteStatement &deleteOutdatedSourceDependenciesStatement = storage.deleteOutdatedSourceDependenciesStatement;
    MockSqliteWriteStatement &deleteNewSourceDependenciesStatement = storage.deleteNewSourceDependenciesStatement;
    MockSqliteReadStatement &getLowestLastModifiedTimeOfDependencies = storage.getLowestLastModifiedTimeOfDependencies;
    MockSqliteWriteStatement &updateBuildDependencyTimeStampStatement = storage.updateBuildDependencyTimeStampStatement;
    MockSqliteWriteStatement &updateSourceTypeStatement = storage.updateSourceTypeStatement;
    MockSqliteReadStatement &fetchSourceDependenciesStatement = storage.fetchSourceDependenciesStatement;
    MockSqliteReadStatement &fetchProjectPartIdStatement = storage.fetchProjectPartIdStatement;
    MockSqliteReadStatement &fetchUsedMacrosStatement = storage.fetchUsedMacrosStatement;
};

TEST_F(BuildDependenciesStorage, ConvertStringsToJson)
{
    Utils::SmallStringVector strings{"foo", "bar", "foo"};

    auto jsonText = storage.toJson(strings);

    ASSERT_THAT(jsonText, Eq("[\"foo\",\"bar\",\"foo\"]"));
}

TEST_F(BuildDependenciesStorage, InsertOrUpdateUsedMacros)
{
    InSequence sequence;

    EXPECT_CALL(insertIntoNewUsedMacrosStatement, write(TypedEq<uint>(42u), TypedEq<Utils::SmallStringView>("FOO")));
    EXPECT_CALL(insertIntoNewUsedMacrosStatement, write(TypedEq<uint>(43u), TypedEq<Utils::SmallStringView>("BAR")));
    EXPECT_CALL(syncNewUsedMacrosStatement, execute());
    EXPECT_CALL(deleteOutdatedUsedMacrosStatement, execute());
    EXPECT_CALL(deleteNewUsedMacrosTableStatement, execute());

    storage.insertOrUpdateUsedMacros({{"FOO", 42}, {"BAR", 43}});
}

TEST_F(BuildDependenciesStorage, InsertFileStatuses)
{
    EXPECT_CALL(insertFileStatuses, write(TypedEq<int>(42), TypedEq<off_t>(1), TypedEq<time_t>(2), TypedEq<bool>(false)));
    EXPECT_CALL(insertFileStatuses, write(TypedEq<int>(43), TypedEq<off_t>(4), TypedEq<time_t>(5), TypedEq<bool>(true)));

    storage.insertFileStatuses({{42, 1, 2, false}, {43, 4, 5, true}});
}

TEST_F(BuildDependenciesStorage, InsertOrUpdateSourceDependencies)
{
    InSequence sequence;

    EXPECT_CALL(insertIntoNewSourceDependenciesStatement, write(TypedEq<int>(42), TypedEq<int>(1)));
    EXPECT_CALL(insertIntoNewSourceDependenciesStatement, write(TypedEq<int>(42), TypedEq<int>(2)));
    EXPECT_CALL(syncNewSourceDependenciesStatement, execute());
    EXPECT_CALL(deleteOutdatedSourceDependenciesStatement, execute());
    EXPECT_CALL(deleteNewSourceDependenciesStatement, execute());

    storage.insertOrUpdateSourceDependencies({{42, 1}, {42, 2}});
}

TEST_F(BuildDependenciesStorage, AddTablesInConstructor)
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


TEST_F(BuildDependenciesStorage, FetchLowestLastModifiedTimeIfNoModificationTimeExists)
{
    EXPECT_CALL(getLowestLastModifiedTimeOfDependencies, valueReturnInt64(Eq(1)));

    auto lowestLastModified = storage.fetchLowestLastModifiedTime(1);

    ASSERT_THAT(lowestLastModified, Eq(0));
}

TEST_F(BuildDependenciesStorage, FetchLowestLastModifiedTime)
{
    EXPECT_CALL(getLowestLastModifiedTimeOfDependencies, valueReturnInt64(Eq(21)))
            .WillRepeatedly(Return(12));

    auto lowestLastModified = storage.fetchLowestLastModifiedTime(21);

    ASSERT_THAT(lowestLastModified, Eq(12));
}

TEST_F(BuildDependenciesStorage, AddNewUsedMacroTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newUsedMacros(sourceId INTEGER, macroName TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newUsedMacros_sourceId_macroName ON newUsedMacros(sourceId, macroName)")));

    storage.createNewUsedMacrosTable();
}

TEST_F(BuildDependenciesStorage, AddNewSourceDependenciesTable)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("CREATE TEMPORARY TABLE newSourceDependencies(sourceId INTEGER, dependencySourceId TEXT)")));
    EXPECT_CALL(mockDatabase, execute(Eq("CREATE INDEX IF NOT EXISTS index_newSourceDependencies_sourceId_dependencySourceId ON newSourceDependencies(sourceId, dependencySourceId)")));

    storage.createNewSourceDependenciesTable();
}

TEST_F(BuildDependenciesStorage, UpdateSources)
{
    InSequence s;
    SourceEntries entries{{1, SourceType::TopInclude, 10}, {2, SourceType::TopSystemInclude, 20}};

    EXPECT_CALL(updateBuildDependencyTimeStampStatement, write(TypedEq<long long>(10), TypedEq<int>(1)));
    EXPECT_CALL(updateSourceTypeStatement, write(TypedEq<uchar>(0), TypedEq<int>(1)));
    EXPECT_CALL(updateBuildDependencyTimeStampStatement, write(TypedEq<long long>(20), TypedEq<int>(2)));
    EXPECT_CALL(updateSourceTypeStatement, write(TypedEq<uchar>(1), TypedEq<int>(2)));

    storage.updateSources(entries);
}

TEST_F(BuildDependenciesStorage, CallsFetchDependSourcesWithNonExistingProjectPartDontFetchesSourceDependencies)
{
    EXPECT_CALL(fetchProjectPartIdStatement, valueReturnInt32(TypedEq<Utils::SmallStringView>("test"))).WillOnce(Return(Utils::optional<int>{}));
    EXPECT_CALL(fetchSourceDependenciesStatement, valuesReturnSourceEntries(_, _, _)).Times(0);

    storage.fetchDependSources(22, "test");
}

TEST_F(BuildDependenciesStorage, CallsFetchDependSourcesWithExistingProjectPartFetchesSourceDependencies)
{
    EXPECT_CALL(fetchProjectPartIdStatement, valueReturnInt32(TypedEq<Utils::SmallStringView>("test"))).WillOnce(Return(Utils::optional<int>{20}));
    EXPECT_CALL(fetchSourceDependenciesStatement, valuesReturnSourceEntries(_, 22, 20));

    storage.fetchDependSources(22, "test");
}

TEST_F(BuildDependenciesStorage, FetchDependSourcesWithNonExistingProjectPartReturnsEmptySourceEntries)
{
    EXPECT_CALL(fetchProjectPartIdStatement, valueReturnInt32(TypedEq<Utils::SmallStringView>("test"))).WillOnce(Return(Utils::optional<int>{}));

    auto entries = storage.fetchDependSources(22, "test");

    ASSERT_THAT(entries, IsEmpty());
}

TEST_F(BuildDependenciesStorage, FetchDependSourcesWithExistingProjectPartReturnsSourceEntries)
{
    SourceEntries sourceEntries{{1, SourceType::TopInclude, 10}, {2, SourceType::TopSystemInclude, 20}};
    EXPECT_CALL(fetchProjectPartIdStatement, valueReturnInt32(TypedEq<Utils::SmallStringView>("test"))).WillOnce(Return(Utils::optional<int>{20}));
    EXPECT_CALL(fetchSourceDependenciesStatement, valuesReturnSourceEntries(_, 22, 20)).WillOnce(Return(sourceEntries));

    auto entries = storage.fetchDependSources(22, "test");

    ASSERT_THAT(entries, sourceEntries);
}

TEST_F(BuildDependenciesStorage, CallsFetchUsedMacros)
{
    EXPECT_CALL(fetchUsedMacrosStatement, valuesReturnUsedMacros(_, 22));

    storage.fetchUsedMacros(22);
}

TEST_F(BuildDependenciesStorage, FetchUsedMacros)
{
    ClangBackEnd::UsedMacros result{UsedMacro{"YI", 1}, UsedMacro{"ER", 2}};
    EXPECT_CALL(fetchUsedMacrosStatement, valuesReturnUsedMacros(_, 22)).WillOnce(Return(result));

    auto usedMacros = storage.fetchUsedMacros(22);

    ASSERT_THAT(usedMacros, result);
}

}

