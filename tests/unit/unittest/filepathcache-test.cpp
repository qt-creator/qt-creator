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

#include <filepathcache.h>

namespace {

using ClangBackEnd::FilePathId;
using FPCB = ClangBackEnd::FilePathCacheBase;
using Cache = ClangBackEnd::FilePathCache<NiceMock<MockFilePathStorage>>;
using ClangBackEnd::FilePathId;

class FilePathCache : public testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockFilePathStorage> mockStorage;
    Cache cache{mockStorage};
};

TEST_F(FilePathCache, FilePathSlashForEmptyPath)
{
    auto slashIndex = FPCB::lastSlashIndex("");

    ASSERT_THAT(slashIndex, -1);
}

TEST_F(FilePathCache, FilePathSlashForSingleSlash)
{
    auto slashIndex = FPCB::lastSlashIndex("/");

    ASSERT_THAT(slashIndex, 0);
}

TEST_F(FilePathCache, FilePathSlashForFileInRoot)
{
    auto slashIndex = FPCB::lastSlashIndex("/file.h");

    ASSERT_THAT(slashIndex, 0);
}

TEST_F(FilePathCache, FilePathSlashForSomeLongerPath)
{
    auto slashIndex = FPCB::lastSlashIndex("/path/to/some/file.h");

    ASSERT_THAT(slashIndex, 13);
}

TEST_F(FilePathCache, FilePathSlashForFileNameOnly)
{
    auto slashIndex = FPCB::lastSlashIndex("file.h");

    ASSERT_THAT(slashIndex, -1);
}

TEST_F(FilePathCache, DirectoryPathForEmptyPath)
{
    auto slashIndex = FPCB::lastSlashIndex("");

    auto directoryPath = FPCB::directoryPath("", slashIndex);

    ASSERT_THAT(directoryPath, "");
}

TEST_F(FilePathCache, DirectoryPathForSingleSlashPath)
{
    Utils::SmallStringView singleSlashPath{"/"};
    auto slashIndex = FPCB::lastSlashIndex(singleSlashPath);

    auto directoryPath = FPCB::directoryPath(singleSlashPath, slashIndex);

    ASSERT_THAT(directoryPath, "");
}

TEST_F(FilePathCache, DirectoryPathForLongerPath)
{
    Utils::SmallStringView longerPath{"/path/to/some/file.h"};
    auto slashIndex = FPCB::lastSlashIndex(longerPath);

    auto directoryPath = FPCB::directoryPath(longerPath, slashIndex);

    ASSERT_THAT(directoryPath, "/path/to/some");
}

TEST_F(FilePathCache, DirectoryPathForFileNameOnly)
{
    Utils::SmallStringView longerPath{"file.h"};
    auto slashIndex = FPCB::lastSlashIndex(longerPath);

    auto directoryPath = FPCB::directoryPath(longerPath, slashIndex);

    ASSERT_THAT(directoryPath, IsEmpty());
}

TEST_F(FilePathCache, FileNameForEmptyPath)
{
    auto slashIndex = FPCB::lastSlashIndex("");

    auto fileName = FPCB::fileName("", slashIndex);

    ASSERT_THAT(fileName, "");
}

TEST_F(FilePathCache, FileNameForSingleSlashPath)
{
    Utils::SmallStringView singleSlashPath{"/"};
    auto slashIndex = FPCB::lastSlashIndex(singleSlashPath);

    auto fileName = FPCB::fileName(singleSlashPath, slashIndex);

    ASSERT_THAT(fileName, "");
}

TEST_F(FilePathCache, FileNameForLongerPath)
{
    Utils::SmallStringView longerPath{"/path/to/some/file.h"};
    auto slashIndex = FPCB::lastSlashIndex(longerPath);

    auto fileName = FPCB::fileName(longerPath, slashIndex);

    ASSERT_THAT(fileName, "file.h");
}

TEST_F(FilePathCache, FileNameForFileNameOnly)
{
    Utils::SmallStringView longerPath{"file.h"};
    auto slashIndex = FPCB::lastSlashIndex(longerPath);

    auto fileName = FPCB::fileName(longerPath, slashIndex);

    ASSERT_THAT(fileName, "file.h");
}

TEST_F(FilePathCache, FilePathIdWithOutAnyEntryCallDirectoryId)
{
    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")));

    cache.filePathId("/path/to/file.cpp");
}

TEST_F(FilePathCache, FilePathIdWithOutAnyEntryCalls)
{
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")));

    cache.filePathId("/path/to/file.cpp");
}

TEST_F(FilePathCache, DirectoryIdOfFilePathIdWithOutAnyEntry)
{
    auto filePathId = cache.filePathId("/path/to/file.cpp");

    ASSERT_THAT(filePathId.directoryId, 5);
}

TEST_F(FilePathCache, FileNameIdOfFilePathIdWithOutAnyEntry)
{
    auto filePathId = cache.filePathId("/path/to/file.cpp");

    ASSERT_THAT(filePathId.fileNameId, 42);
}

TEST_F(FilePathCache,  IfEntryExistsDontCallInStrorage)
{
    cache.filePathId("/path/to/file.cpp");

    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp"))).Times(0);

    cache.filePathId("/path/to/file.cpp");
}

TEST_F(FilePathCache,  IfDirectoryEntryExistsDontCallFetchDirectoryIdButStillCallFetchSourceId)
{
    cache.filePathId("/path/to/file2.cpp");

    EXPECT_CALL(mockStorage, fetchDirectoryId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")));

    cache.filePathId("/path/to/file.cpp");
}

TEST_F(FilePathCache, GetFileNameIdWithCachedValue)
{
    cache.filePathId("/path/to/file.cpp");

    auto filePathId = cache.filePathId("/path/to/file.cpp");

    ASSERT_THAT(filePathId.fileNameId, 42);
}

TEST_F(FilePathCache, GetFileNameIdWithDirectoryIdCached)
{
    cache.filePathId("/path/to/file.cpp");

    auto filePathId = cache.filePathId("/path/to/file2.cpp");

    ASSERT_THAT(filePathId.fileNameId, 63);
}

TEST_F(FilePathCache, GetDirectyIdWithCachedValue)
{
    cache.filePathId("/path/to/file.cpp");

    auto filePathId = cache.filePathId("/path/to/file2.cpp");

    ASSERT_THAT(filePathId.directoryId, 5);
}

TEST_F(FilePathCache, GetDirectyIdWithDirectoryIdCached)
{
    cache.filePathId("/path/to/file.cpp");

    auto filePathId = cache.filePathId("/path/to/file2.cpp");

    ASSERT_THAT(filePathId.directoryId, 5);
}

TEST_F(FilePathCache, ThrowForGettingAFilePathWithAnInvalidId)
{
    FilePathId filePathId;

    ASSERT_THROW(cache.filePath(filePathId), ClangBackEnd::NoFilePathForInvalidFilePathId);
}

TEST_F(FilePathCache, GetAFilePath)
{
    FilePathId filePathId = cache.filePathId("/path/to/file.cpp");

    auto filePath = cache.filePath(filePathId);

    ASSERT_THAT(filePath, Eq("/path/to/file.cpp"));
}

TEST_F(FilePathCache, GetAFilePathWithCachedFilePathId)
{
    FilePathId filePathId{5, 42};

    auto filePath = cache.filePath(filePathId);

    ASSERT_THAT(filePath, Eq("/path/to/file.cpp"));
}

void FilePathCache::SetUp()
{
    ON_CALL(mockStorage, fetchDirectoryId(Eq("/path/to")))
            .WillByDefault(Return(5));
    ON_CALL(mockStorage, fetchSourceId(5, Eq("file.cpp")))
            .WillByDefault(Return(42));
    ON_CALL(mockStorage, fetchSourceId(5, Eq("file2.cpp")))
            .WillByDefault(Return(63));
    ON_CALL(mockStorage, fetchDirectoryPath(5))
            .WillByDefault(Return(Utils::PathString("/path/to")));
    ON_CALL(mockStorage, fetchSourceName(42))
            .WillByDefault(Return(Utils::SmallString("file.cpp")));
}

}

