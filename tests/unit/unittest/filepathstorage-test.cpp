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

#include "mockmutex.h"
#include "mocksqlitedatabase.h"
#include "mocksqlitereadstatement.h"
#include "mocksqlitewritestatement.h"

#include <filepathstorage.h>
#include <filepathstoragesqlitestatementfactory.h>

namespace {

using StatementFactory = ClangBackEnd::FilePathStorageSqliteStatementFactory<NiceMock<MockSqliteDatabase>,
                                                                             MockSqliteReadStatement,
                                                                             MockSqliteWriteStatement>;
using Storage = ClangBackEnd::FilePathStorage<StatementFactory>;
using ClangBackEnd::Sources::Directory;
using ClangBackEnd::Sources::Source;

class FilePathStorage : public testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockMutex> mockMutex;
    NiceMock<MockSqliteDatabase> mockDatabase{mockMutex};
    StatementFactory factory{mockDatabase};
    MockSqliteReadStatement &selectDirectoryIdFromDirectoriesByDirectoryPath = factory.selectDirectoryIdFromDirectoriesByDirectoryPath;
    MockSqliteReadStatement &selectSourceIdFromSourcesByDirectoryIdAndSourceName = factory.selectSourceIdFromSourcesByDirectoryIdAndSourceName;
    MockSqliteReadStatement &selectDirectoryPathFromDirectoriesByDirectoryId = factory.selectDirectoryPathFromDirectoriesByDirectoryId;
    MockSqliteReadStatement &selectSourceNameFromSourcesBySourceId = factory.selectSourceNameFromSourcesBySourceId;
    MockSqliteReadStatement &selectAllDirectories = factory.selectAllDirectories;
    MockSqliteWriteStatement &insertIntoDirectories = factory.insertIntoDirectories;
    MockSqliteWriteStatement &insertIntoSources = factory.insertIntoSources;
    MockSqliteReadStatement &selectAllSources = factory.selectAllSources;
    Storage storage{factory};
};

TEST_F(FilePathStorage, ReadDirectoryIdForNotContainedPath)
{
    auto directoryId = storage.readDirectoryId("/some/not/known/path");

    ASSERT_FALSE(directoryId);
}

TEST_F(FilePathStorage, ReadSourceIdForNotContainedPathAndDirectoryId)
{
    auto sourceId = storage.readSourceId(23, "/some/not/known/path");

    ASSERT_FALSE(sourceId);
}

TEST_F(FilePathStorage, ReadDirectoryIdForEmptyPath)
{
    auto directoryId = storage.readDirectoryId("");

    ASSERT_THAT(directoryId.value(), 0);
}

TEST_F(FilePathStorage, ReadSourceIdForEmptyNameAndZeroDirectoryId)
{
    auto sourceId = storage.readSourceId(0, "");

    ASSERT_THAT(sourceId.value(), 0);
}

TEST_F(FilePathStorage, ReadDirectoryIdForPath)
{
    auto directoryId = storage.readDirectoryId("/path/to");

    ASSERT_THAT(directoryId.value(), 5);
}

TEST_F(FilePathStorage, ReadSourceIdForPathAndDirectoryId)
{
    auto sourceId = storage.readSourceId(5, "file.h");

    ASSERT_THAT(sourceId.value(), 42);
}

TEST_F(FilePathStorage, FetchDirectoryIdForEmptyPath)
{
    auto directoryId = storage.fetchDirectoryId("");

    ASSERT_THAT(directoryId, 0);
}

TEST_F(FilePathStorage, FetchSourceIdForEmptyNameAndZeroDirectoryId)
{
    auto sourceId = storage.fetchSourceId(0, "");

    ASSERT_THAT(sourceId, 0);
}

TEST_F(FilePathStorage, FetchDirectoryIdForPath)
{
    auto directoryId = storage.fetchDirectoryId("/path/to");

    ASSERT_THAT(directoryId, 5);
}

TEST_F(FilePathStorage, FetchSourceIdForPathAndDirectoryId)
{
    auto sourceId = storage.fetchSourceId(5, "file.h");

    ASSERT_THAT(sourceId, 42);
}

TEST_F(FilePathStorage, CallWriteForWriteDirectory)
{
    EXPECT_CALL(insertIntoDirectories, write(Eq("/some/not/known/path")));

    storage.writeDirectoryId("/some/not/known/path");
}

TEST_F(FilePathStorage, CallWriteForWriteSource)
{
    EXPECT_CALL(insertIntoSources, write(5, Eq("unknownfile.h")));

    storage.writeSourceId(5, "unknownfile.h");
}

TEST_F(FilePathStorage, GetTheDirectoryIdBackAfterWritingANewEntryInDirectories)
{
    auto directoryId = storage.writeDirectoryId("/some/not/known/path");

    ASSERT_THAT(directoryId, 12);
}

TEST_F(FilePathStorage, GetTheSourceIdBackAfterWritingANewEntryInSources)
{
    auto sourceId = storage.writeSourceId(5, "unknownfile.h");

    ASSERT_THAT(sourceId, 12);
}

TEST_F(FilePathStorage, GetTheDirectoryIdBackAfterFetchingANewEntryFromDirectories)
{
    auto directoryId = storage.fetchDirectoryId("/some/not/known/path");

    ASSERT_THAT(directoryId, 12);
}

TEST_F(FilePathStorage, GetTheSourceIdBackAfterFetchingANewEntryFromSources)
{
    auto sourceId = storage.fetchSourceId(5, "unknownfile.h");

    ASSERT_THAT(sourceId, 12);
}

TEST_F(FilePathStorage, CallSelectForFetchingDirectoryIdForKnownPath)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
                valueReturnInt32(Eq("/path/to")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchDirectoryId("/path/to");
}

TEST_F(FilePathStorage, CallSelectForFetchingSourceIdForKnownPath)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
                valueReturnInt32(5, Eq("file.h")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchSourceId(5, "file.h");
}

TEST_F(FilePathStorage, CallNotWriteForFetchingDirectoryIdForKnownPath)
{
    EXPECT_CALL(insertIntoDirectories, write(_)).Times(0);

    storage.fetchDirectoryId("/path/to");
}

TEST_F(FilePathStorage, CallNotWriteForFetchingSoureIdForKnownEntry)
{
    EXPECT_CALL(insertIntoSources, write(_, _)).Times(0);

    storage.fetchSourceId(5, "file.h");
}

TEST_F(FilePathStorage, CallSelectAndWriteForFetchingDirectoryIdForUnknownPath)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
                valueReturnInt32(Eq("/some/not/known/path")));
    EXPECT_CALL(insertIntoDirectories, write(Eq("/some/not/known/path")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchDirectoryId("/some/not/known/path");
}

TEST_F(FilePathStorage, CallSelectAndWriteForFetchingSourceIdForUnknownEntry)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
                valueReturnInt32(5, Eq("unknownfile.h")));
    EXPECT_CALL(insertIntoSources, write(5, Eq("unknownfile.h")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchSourceId(5, "unknownfile.h");
}

TEST_F(FilePathStorage, CallSelectAndWriteForFetchingDirectoryIdTwoTimesIfTheDatabaseIsBusyBecauseTheTableAlreadyChanged)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
                valueReturnInt32(Eq("/other/unknow/path")));
    EXPECT_CALL(insertIntoDirectories, write(Eq("/other/unknow/path")))
            .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(mockDatabase, execute(Eq("ROLLBACK")));
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
                valueReturnInt32(Eq("/other/unknow/path")));
    EXPECT_CALL(insertIntoDirectories, write(Eq("/other/unknow/path")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchDirectoryId("/other/unknow/path");
}


TEST_F(FilePathStorage, CallSelectAndWriteForFetchingSourceTwoTimesIfTheDatabaseIsBusyBecauseTheTableAlreadyChanged)
{
    InSequence s;

    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
                valueReturnInt32(5, Eq("otherunknownfile.h")));
    EXPECT_CALL(insertIntoSources, write(5, Eq("otherunknownfile.h")))
            .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));;
    EXPECT_CALL(mockDatabase, execute(Eq("ROLLBACK")));
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
                valueReturnInt32(5, Eq("otherunknownfile.h")));
    EXPECT_CALL(insertIntoSources, write(5, Eq("otherunknownfile.h")));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchSourceId(5, "otherunknownfile.h");
}

TEST_F(FilePathStorage, SelectAllDirectories)
{
    auto directories = storage.fetchAllDirectories();

    ASSERT_THAT(directories,
                ElementsAre(Directory{1, "/path/to"}, Directory{2, "/other/path"}));
}

TEST_F(FilePathStorage, SelectAllSources)
{
    auto sources = storage.fetchAllSources();

    ASSERT_THAT(sources,
                ElementsAre(Source{1, "file.h"}, Source{4, "file.cpp"}));
}

TEST_F(FilePathStorage, CallSelectAllDirectories)
{
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectAllDirectories, valuesReturnStdVectorDirectory(256));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchAllDirectories();
}

TEST_F(FilePathStorage, CallSelectAllSources)
{
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectAllSources, valuesReturnStdVectorSource(8192));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchAllSources();
}

TEST_F(FilePathStorage, CallValueForFetchDirectoryPathForId)
{
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectDirectoryPathFromDirectoriesByDirectoryId, valueReturnPathString(5));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchDirectoryPath(5);
}

TEST_F(FilePathStorage, FetchDirectoryPathForId)
{
    auto path = storage.fetchDirectoryPath(5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(FilePathStorage, ThrowAsFetchingDirectoryPathForNonExistingId)
{
    ASSERT_THROW(storage.fetchDirectoryPath(12), ClangBackEnd::DirectoryPathIdDoesNotExists);
}

TEST_F(FilePathStorage, CallValueForFetchSoureNameForId)
{
    EXPECT_CALL(mockDatabase, execute(Eq("BEGIN")));
    EXPECT_CALL(selectSourceNameFromSourcesBySourceId, valueReturnSmallString(42));
    EXPECT_CALL(mockDatabase, execute(Eq("COMMIT")));

    storage.fetchSourceName(42);
}

TEST_F(FilePathStorage, FetchSoureNameForId)
{
    auto path = storage.fetchSourceName(42);

    ASSERT_THAT(path, Eq("file.cpp"));
}

TEST_F(FilePathStorage, ThrowAsFetchingSourceNameForNonExistingId)
{
    ASSERT_THROW(storage.fetchSourceName(12), ClangBackEnd::SourceNameIdDoesNotExists);
}

void FilePathStorage::SetUp()
{
    ON_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
            valueReturnInt32(_))
            .WillByDefault(Return(Utils::optional<int>()));
    ON_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
            valueReturnInt32(Utils::SmallStringView("")))
            .WillByDefault(Return(Utils::optional<int>(0)));
    ON_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath,
            valueReturnInt32(Utils::SmallStringView("/path/to")))
            .WillByDefault(Return(Utils::optional<int>(5)));
    ON_CALL(mockDatabase, lastInsertedRowId())
            .WillByDefault(Return(12));
    ON_CALL(selectAllDirectories,
            valuesReturnStdVectorDirectory(_))
            .WillByDefault(Return(std::vector<Directory>{{1, "/path/to"}, {2, "/other/path"}}));
    ON_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
            valueReturnInt32(_, _))
            .WillByDefault(Return(Utils::optional<int>()));
    ON_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
            valueReturnInt32(0, Utils::SmallStringView("")))
            .WillByDefault(Return(Utils::optional<int>(0)));
    ON_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName,
            valueReturnInt32(5, Utils::SmallStringView("file.h")))
            .WillByDefault(Return(Utils::optional<int>(42)));
    ON_CALL(selectAllSources,
            valuesReturnStdVectorSource(_))
            .WillByDefault(Return(std::vector<Source>{{1, "file.h"}, {4, "file.cpp"}}));
    ON_CALL(selectDirectoryPathFromDirectoriesByDirectoryId,
            valueReturnPathString(5))
            .WillByDefault(Return(Utils::optional<Utils::PathString>("/path/to")));
    ON_CALL(selectSourceNameFromSourcesBySourceId,
            valueReturnSmallString(42))
            .WillByDefault(Return(Utils::optional<Utils::SmallString>("file.cpp")));


    EXPECT_CALL(selectDirectoryIdFromDirectoriesByDirectoryPath, valueReturnInt32(_))
            .Times(AnyNumber());
    EXPECT_CALL(selectSourceIdFromSourcesByDirectoryIdAndSourceName, valueReturnInt32(_, _))
            .Times(AnyNumber());
    EXPECT_CALL(insertIntoDirectories,write(_))
            .Times(AnyNumber());
    EXPECT_CALL(insertIntoSources,write(_, _))
            .Times(AnyNumber());
    EXPECT_CALL(selectAllDirectories, valuesReturnStdVectorDirectory(_))
            .Times(AnyNumber());
    EXPECT_CALL(selectAllSources, valuesReturnStdVectorSource(_))
            .Times(AnyNumber());
    EXPECT_CALL(selectDirectoryPathFromDirectoriesByDirectoryId, valueReturnPathString(_))
            .Times(AnyNumber());
    EXPECT_CALL(selectSourceNameFromSourcesBySourceId, valueReturnSmallString(_))
            .Times(AnyNumber());
}
}
