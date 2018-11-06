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
using Cache = ClangBackEnd::FilePathCache<NiceMock<MockFilePathStorage>>;
using ClangBackEnd::FilePathId;
using NFP = ClangBackEnd::FilePath;
using ClangBackEnd::FilePathView;
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
        ON_CALL(mockStorage, fetchDirectoryPath(5))
                .WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(mockStorage, fetchSourceNameAndDirectoryId(42))
                .WillByDefault(Return(SourceNameAndDirectoryId("file.cpp", 5)));
    }

protected:
    NiceMock<MockFilePathStorage> mockStorage;
    Cache cache{mockStorage};
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

}
