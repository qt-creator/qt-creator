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

#include "mockfilepathstorage.h"
#include "mocksqlitedatabase.h"

#include <filepathcache.h>

namespace {

using ClangBackEnd::DirectoryPathId;
using ClangBackEnd::FilePathId;
using Cache = ClangBackEnd::FilePathCache<NiceMock<MockFilePathStorage>>;
using ClangBackEnd::FilePathId;
using NFP = ClangBackEnd::FilePath;
using ClangBackEnd::FilePathView;
using ClangBackEnd::FilePathViews;
using ClangBackEnd::Sources::SourceNameAndDirectoryId;

class FilePathCache : public testing::Test
{
protected:
    void SetUp()
    {
        ON_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")))
                .WillByDefault(Return(5));
        ON_CALL(mockStorage, fetchDirectoryId(Eq("/path2/to")))
                .WillByDefault(Return(6));
        ON_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")))
                .WillByDefault(Return(42));
        ON_CALL(mockStorage, fetchSourceId(5, Eq("file2.cpp")))
                .WillByDefault(Return(63));
        ON_CALL(mockStorage, fetchSourceId(6, Eq("file.cpp")))
                .WillByDefault(Return(72));
        ON_CALL(mockStorage, fetchDirectoryPath(5)).WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(mockStorage, fetchSourceNameAndDirectoryId(42))
            .WillByDefault(Return(SourceNameAndDirectoryId("file.cpp", 5)));
        ON_CALL(mockStorageFilled, fetchAllSources())
            .WillByDefault(Return(std::vector<ClangBackEnd::Sources::Source>({
                {"file.cpp", 6, 72},
                {"file2.cpp", 5, 63},
                {"file.cpp", 5, 42},
            })));
        ON_CALL(mockStorageFilled, fetchAllDirectories())
            .WillByDefault(Return(
                std::vector<ClangBackEnd::Sources::Directory>({{"/path2/to", 6}, {"/path/to", 5}})));
        ON_CALL(mockStorageFilled, fetchDirectoryIdUnguarded(Eq("/path3/to"))).WillByDefault(Return(7));
        ON_CALL(mockStorageFilled, fetchSourceIdUnguarded(7, Eq("file.h"))).WillByDefault(Return(101));
        ON_CALL(mockStorageFilled, fetchSourceIdUnguarded(6, Eq("file2.h"))).WillByDefault(Return(106));
        ON_CALL(mockStorageFilled, fetchSourceIdUnguarded(5, Eq("file.h"))).WillByDefault(Return(99));
    }

protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    NiceMock<MockFilePathStorage> mockStorage{mockDatabase};
    Cache cache{mockStorage};
    NiceMock<MockFilePathStorage> mockStorageFilled{mockDatabase};
    Cache cacheNotFilled{mockStorageFilled};
};

TEST_F(FilePathCache, FilePathIdWithOutAnyEntryCallDirectoryId)
{
    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")));

    cache.filePathId(FilePathView("/path/to/file.cpp"));
}

TEST_F(FilePathCache, FilePathIdWithOutAnyEntryCalls)
{
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")));

    cache.filePathId(FilePathView("/path/to/file.cpp"));
}

TEST_F(FilePathCache, FilePathIdOfFilePathIdWithOutAnyEntry)
{
    auto filePathId = cache.filePathId(FilePathView("/path/to/file.cpp"));

    ASSERT_THAT(filePathId.filePathId, 42);
}

TEST_F(FilePathCache,  IfEntryExistsDontCallInStrorage)
{
    cache.filePathId(FilePathView("/path/to/file.cpp"));

    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp"))).Times(0);

    cache.filePathId(FilePathView("/path/to/file.cpp"));
}

TEST_F(FilePathCache,  IfDirectoryEntryExistsDontCallFetchDirectoryIdButStillCallFetchSourceId)
{
    cache.filePathId(FilePathView("/path/to/file2.cpp"));

    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")));

    cache.filePathId(FilePathView("/path/to/file.cpp"));
}

TEST_F(FilePathCache, GetFilePathIdWithCachedValue)
{
    cache.filePathId(FilePathView("/path/to/file.cpp"));

    auto filePathId = cache.filePathId(FilePathView("/path/to/file.cpp"));

    ASSERT_THAT(filePathId.filePathId, 42);
}

TEST_F(FilePathCache, GetFilePathIdWithDirectoryIdCached)
{
    cache.filePathId(FilePathView("/path/to/file.cpp"));

    auto filePathId = cache.filePathId(FilePathView("/path/to/file2.cpp"));

    ASSERT_THAT(filePathId.filePathId, 63);
}

TEST_F(FilePathCache, ThrowForGettingAFilePathWithAnInvalidId)
{
    FilePathId filePathId;

    ASSERT_THROW(cache.filePath(filePathId), ClangBackEnd::NoFilePathForInvalidFilePathId);
}

TEST_F(FilePathCache, GetAFilePath)
{
    FilePathId filePathId = cache.filePathId(FilePathView("/path/to/file.cpp"));

    auto filePath = cache.filePath(filePathId);

    ASSERT_THAT(filePath, Eq(FilePathView{"/path/to/file.cpp"}));
}

TEST_F(FilePathCache, GetAFilePathWithCachedFilePathId)
{
    FilePathId filePathId{42};

    auto filePath = cache.filePath(filePathId);

    ASSERT_THAT(filePath, Eq(FilePathView{"/path/to/file.cpp"}));
}

TEST_F(FilePathCache, FileNamesAreUniqueForEveryDirectory)
{
    FilePathId filePathId = cache.filePathId(FilePathView("/path/to/file.cpp"));

    FilePathId filePath2Id = cache.filePathId(FilePathView("/path2/to/file.cpp"));

    ASSERT_THAT(filePath2Id.filePathId, Ne(filePathId.filePathId));
}

TEST_F(FilePathCache, DuplicateFilePathsAreEqual)
{
    FilePathId filePath1Id = cache.filePathId(FilePathView("/path/to/file.cpp"));

    FilePathId filePath2Id = cache.filePathId(FilePathView("/path/to/file.cpp"));

    ASSERT_THAT(filePath2Id, Eq(filePath1Id));
}

TEST_F(FilePathCache, DirectoryPathIdCallsFetchDirectoryId)
{
    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")));

    cache.directoryPathId(Utils::SmallString("/path/to"));
}

TEST_F(FilePathCache, SecondDirectoryPathIdCallsNotFetchDirectoryId)
{
    cache.directoryPathId(Utils::SmallString("/path/to"));

    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to"))).Times(0);

    cache.directoryPathId(Utils::SmallString("/path/to"));
}

TEST_F(FilePathCache, DirectoryPathIdWithTrailingSlash)
{
    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")));

    cache.directoryPathId(Utils::SmallString("/path/to/"));
}

TEST_F(FilePathCache, DirectoryPathId)
{
    auto id = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(id, Eq(DirectoryPathId{5}));
}

TEST_F(FilePathCache, DirectoryPathIdIsAlreadyInCache)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(FilePathCache, DirectoryPathIdIsAlreadyInCacheWithTrailingSlash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(FilePathCache, DirectoryPathIdIsAlreadyInCacheWithAndWithoutTrailingSlash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(FilePathCache, DirectoryPathIdIsAlreadyInCacheWithoutAndWithTrailingSlash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(FilePathCache, ThrowForGettingADirectoryPathWithAnInvalidId)
{
    DirectoryPathId directoryPathId;

    ASSERT_THROW(cache.directoryPath(directoryPathId),
                 ClangBackEnd::NoDirectoryPathForInvalidDirectoryPathId);
}

TEST_F(FilePathCache, GetADirectoryPath)
{
    DirectoryPathId directoryPathId{5};

    auto directoryPath = cache.directoryPath(directoryPathId);

    ASSERT_THAT(directoryPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(FilePathCache, GetADirectoryPathWithCachedDirectoryPathId)
{
    DirectoryPathId directoryPathId{5};
    cache.directoryPath(directoryPathId);

    auto directoryPath = cache.directoryPath(directoryPathId);

    ASSERT_THAT(directoryPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(FilePathCache, DirectoryPathCallsFetchDirectoryPath)
{
    EXPECT_CALL(mockStorage, fetchDirectoryPath(Eq(DirectoryPathId{5})));

    cache.directoryPath(5);
}

TEST_F(FilePathCache, SecondDirectoryPathCallsNotFetchDirectoryPath)
{
    cache.directoryPath(5);

    EXPECT_CALL(mockStorage, fetchDirectoryPath(_)).Times(0);

    cache.directoryPath(5);
}

TEST_F(FilePathCache, ThrowForGettingADirectoryPathIdWithAnInvalidFilePathId)
{
    FilePathId filePathId;

    ASSERT_THROW(cache.directoryPathId(filePathId), ClangBackEnd::NoFilePathForInvalidFilePathId);
}

TEST_F(FilePathCache, FetchDirectoryPathIdByFilePathId)
{
    auto directoryId = cache.directoryPathId(42);

    ASSERT_THAT(directoryId, Eq(5));
}

TEST_F(FilePathCache, FetchDirectoryPathIdByFilePathIdCached)
{
    cache.directoryPathId(42);

    auto directoryId = cache.directoryPathId(42);

    ASSERT_THAT(directoryId, Eq(5));
}

TEST_F(FilePathCache, FetchFilePathAfterFetchingDirectoryIdByFilePathId)
{
    cache.directoryPathId(42);

    auto filePath = cache.filePath(42);

    ASSERT_THAT(filePath, Eq("/path/to/file.cpp"));
}

TEST_F(FilePathCache, FetchDirectoryPathIdAfterFetchingFilePathByFilePathId)
{
    cache.filePath(42);

    auto directoryId = cache.directoryPathId(42);

    ASSERT_THAT(directoryId, Eq(5));
}

TEST_F(FilePathCache, FetchAllDirectoriesAndSourcesAtCreation)
{
    EXPECT_CALL(mockStorage, fetchAllDirectories());
    EXPECT_CALL(mockStorage, fetchAllSources());

    Cache cache{mockStorage};
}

TEST_F(FilePathCache, GetFileIdInFilledCache)
{
    Cache cacheFilled{mockStorageFilled};

    auto id = cacheFilled.filePathId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(72));
}

TEST_F(FilePathCache, GetDirectoryIdInFilledCache)
{
    Cache cacheFilled{mockStorageFilled};

    auto id = cacheFilled.directoryPathId(42);

    ASSERT_THAT(id, Eq(5));
}

TEST_F(FilePathCache, GetDirectoryPathInFilledCache)
{
    Cache cacheFilled{mockStorageFilled};

    auto path = cacheFilled.directoryPath(5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(FilePathCache, GetFilePathInFilledCache)
{
    Cache cacheFilled{mockStorageFilled};

    auto path = cacheFilled.filePath(42);

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(FilePathCache, GetFileIdAfterAddFilePaths)
{
    Cache cacheFilled{mockStorageFilled};

    cacheFilled.addFilePaths(
        FilePathViews{"/path3/to/file.h", "/path/to/file.h", "/path2/to/file2.h", "/path/to/file.cpp"});

    ASSERT_THAT(cacheFilled.filePath(101), Eq("/path3/to/file.h"));
}

TEST_F(FilePathCache, GetFileIdAfterAddFilePathsWhichWasAlreadyAdded)
{
    Cache cacheFilled{mockStorageFilled};

    cacheFilled.addFilePaths(FilePathViews{"/path3/to/file.h", "/path/to/file.h", "/path2/to/file2.h"});

    ASSERT_THAT(cacheFilled.filePath(42), Eq("/path/to/file.cpp"));
}

TEST_F(FilePathCache, AddFilePathsCalls)
{
    Cache cacheFilled{mockStorageFilled};
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(mockStorageFilled, fetchDirectoryIdUnguarded(Eq("/path3/to"))).WillOnce(Return(7));
    EXPECT_CALL(mockStorageFilled, fetchDirectoryIdUnguarded(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(5, Eq("file.h"))).WillOnce(Return(99));
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(6, Eq("file2.h"))).WillOnce(Return(106));
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(7, Eq("file.h"))).WillOnce(Return(101));
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(5, Eq("file.cpp"))).Times(0);
    EXPECT_CALL(mockDatabase, commit());

    cacheFilled.addFilePaths(
        FilePathViews{"/path3/to/file.h", "/path/to/file.h", "/path2/to/file2.h", "/path/to/file.cpp"});
}

TEST_F(FilePathCache, DontUseTransactionIfNotAddingFilesInAddFilePathsCalls)
{
    Cache cacheFilled{mockStorageFilled};
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin()).Times(0);
    EXPECT_CALL(mockStorageFilled, fetchDirectoryIdUnguarded(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(5, Eq("file.cpp"))).Times(0);
    EXPECT_CALL(mockDatabase, commit()).Times(0);

    cacheFilled.addFilePaths(FilePathViews{"/path/to/file.cpp"});
}

TEST_F(FilePathCache, UseTransactionIfAddingFilesOnlyInAddFilePathsCalls)
{
    Cache cacheFilled{mockStorageFilled};
    InSequence s;

    EXPECT_CALL(mockDatabase, deferredBegin());
    EXPECT_CALL(mockStorageFilled, fetchDirectoryIdUnguarded(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorageFilled, fetchSourceIdUnguarded(5, Eq("file.h")));
    EXPECT_CALL(mockDatabase, commit());

    cacheFilled.addFilePaths(FilePathViews{"/path/to/file.h"});
}

TEST_F(FilePathCache, GetFileIdInAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.filePathId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(72));
}

TEST_F(FilePathCache, DontPopulateIfNotEmpty)
{
    cacheNotFilled.filePathId("/path/to/file.cpp");

    EXPECT_CALL(mockStorageFilled, fetchAllDirectories()).Times(0);
    EXPECT_CALL(mockStorageFilled, fetchAllSources()).Times(0);

    cacheNotFilled.populateIfEmpty();
}

TEST_F(FilePathCache, GetDirectoryIdAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.directoryPathId(42);

    ASSERT_THAT(id, Eq(5));
}

TEST_F(FilePathCache, GetDirectoryPathAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.directoryPath(5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(FilePathCache, GetFilePathAfterPopulateIfEmptye)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.filePath(42);

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

} // namespace
