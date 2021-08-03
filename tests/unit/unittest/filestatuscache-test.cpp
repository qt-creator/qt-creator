/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "filesystemmock.h"
#include "googletest.h"

#include <projectstorage/filestatuscache.h>

#include <QDateTime>
#include <QFileInfo>

#include <fstream>

namespace {

using QmlDesigner::FileStatus;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;

class FileStatusCache : public testing::Test
{
protected:
    FileStatusCache()
    {
        ON_CALL(fileSystem, fileStatus(Eq(header)))
            .WillByDefault(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}));
        ON_CALL(fileSystem, fileStatus(Eq(source)))
            .WillByDefault(Return(FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
        ON_CALL(fileSystem, fileStatus(Eq(header2)))
            .WillByDefault(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}));
        ON_CALL(fileSystem, fileStatus(Eq(source2)))
            .WillByDefault(Return(FileStatus{source2, source2FileSize, source2LastModifiedTime}));
    }

protected:
    NiceMock<FileSystemMock> fileSystem;
    QmlDesigner::FileStatusCache cache{fileSystem};
    SourceId header{1};
    SourceId source{2};
    SourceId header2{3};
    SourceId source2{4};
    SourceIds entries{header, source, header2, source2};
    long long headerLastModifiedTime = 100;
    long long headerLastModifiedTime2 = 110;
    long long headerFileSize = 1000;
    long long headerFileSize2 = 1100;
    long long header2LastModifiedTime = 300;
    long long header2LastModifiedTime2 = 310;
    long long header2FileSize = 3000;
    long long header2FileSize2 = 3100;
    long long sourceLastModifiedTime = 200;
    long long source2LastModifiedTime = 400;
    long long sourceFileSize = 2000;
    long long source2FileSize = 4000;
};

TEST_F(FileStatusCache, CreateEntry)
{
    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, AskCreatedEntryForLastModifiedTime)
{
    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, FindCachedEntry)
{
    cache.find(header);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, LastModifiedTime)
{
    cache.find(header);

    auto lastModifiedTime = cache.lastModifiedTime(header);

    ASSERT_THAT(lastModifiedTime, headerLastModifiedTime);
}

TEST_F(FileStatusCache, FileSize)
{
    cache.find(header);

    auto fileSize = cache.fileSize(header);

    ASSERT_THAT(fileSize, headerFileSize);
}

TEST_F(FileStatusCache, DontAddEntryTwice)
{
    cache.find(header);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, AddNewEntry)
{
    cache.find(header);

    cache.find(source);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, AskNewEntryForLastModifiedTime)
{
    cache.find(header);

    auto fileStatus = cache.find(source);

    ASSERT_THAT(fileStatus, (FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
}

TEST_F(FileStatusCache, AddNewEntryReverseOrder)
{
    cache.find(source);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, AskNewEntryReverseOrderAddedForLastModifiedTime)
{
    cache.find(source);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, UpdateFile)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache.find(header), (FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
}

TEST_F(FileStatusCache, UpdateFileDoesNotChangeEntryCount)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime2}));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, UpdateFileForNonExistingEntry)
{
    cache.update(header);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, UpdateFileStats)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}))
        .WillOnce(Return(FileStatus{header2, header2FileSize2, header2LastModifiedTime2}));
    cache.lastModifiedTime(header);
    cache.lastModifiedTime(header2);

    cache.update(entries);

    ASSERT_THAT(cache.find(header), (FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
    ASSERT_THAT(cache.find(header2),
                (FileStatus{header2, header2FileSize2, header2LastModifiedTime2}));
}

TEST_F(FileStatusCache, UpdateFilesDoesNotChangeEntryCount)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}))
        .WillOnce(Return(FileStatus{header2, header2FileSize2, header2LastModifiedTime}));
    cache.lastModifiedTime(header);
    cache.lastModifiedTime(header2);

    cache.update(entries);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, UpdateFilesForNonExistingEntry)
{
    cache.update(entries);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, NewModifiedEntries)
{
    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, entries);
}

TEST_F(FileStatusCache, NoNewModifiedEntries)
{
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, IsEmpty());
}

TEST_F(FileStatusCache, SomeNewModifiedEntries)
{
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, source2));
}

TEST_F(FileStatusCache, SomeAlreadyExistingModifiedEntries)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}))
        .WillOnce(Return(FileStatus{header2, header2FileSize2, header2LastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(FileStatus{source2, source2FileSize, source2LastModifiedTime}));
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2));
}

TEST_F(FileStatusCache, SomeAlreadyExistingAndSomeNewModifiedEntries)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .WillRepeatedly(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}))
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime2}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .WillRepeatedly(Return(FileStatus{source2, source2FileSize, source2LastModifiedTime}));
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2, source2));
}

TEST_F(FileStatusCache, TimeIsUpdatedForSomeAlreadyExistingModifiedEntries)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(FileStatus{header2, header2FileSize, header2LastModifiedTime}))
        .WillOnce(Return(FileStatus{header2, header2FileSize2, header2LastModifiedTime2}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(FileStatus{source2, source2FileSize, source2LastModifiedTime}));
    cache.modified(entries);

    cache.modified(entries);

    ASSERT_THAT(cache.find(header), (FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
}

} // namespace
