// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../mocks/filesystemmock.h"
#include "../utils/googletest.h"

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
    SourceId header{SourceId::create(1)};
    SourceId source{SourceId::create(2)};
    SourceId header2{SourceId::create(3)};
    SourceId source2{SourceId::create(4)};
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

TEST_F(FileStatusCache, create_entry)
{
    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, ask_created_entry_for_last_modified_time)
{
    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, find_cached_entry)
{
    cache.find(header);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, last_modified_time)
{
    cache.find(header);

    auto lastModifiedTime = cache.lastModifiedTime(header);

    ASSERT_THAT(lastModifiedTime, headerLastModifiedTime);
}

TEST_F(FileStatusCache, file_size)
{
    cache.find(header);

    auto fileSize = cache.fileSize(header);

    ASSERT_THAT(fileSize, headerFileSize);
}

TEST_F(FileStatusCache, dont_add_entry_twice)
{
    cache.find(header);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, add_new_entry)
{
    cache.find(header);

    cache.find(source);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, ask_new_entry_for_last_modified_time)
{
    cache.find(header);

    auto fileStatus = cache.find(source);

    ASSERT_THAT(fileStatus, (FileStatus{source, sourceFileSize, sourceLastModifiedTime}));
}

TEST_F(FileStatusCache, add_new_entry_reverse_order)
{
    cache.find(source);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, ask_new_entry_reverse_order_added_for_last_modified_time)
{
    cache.find(source);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (FileStatus{header, headerFileSize, headerLastModifiedTime}));
}

TEST_F(FileStatusCache, update_file)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache.find(header), (FileStatus{header, headerFileSize2, headerLastModifiedTime2}));
}

TEST_F(FileStatusCache, update_file_does_not_change_entry_count)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime}))
        .WillOnce(Return(FileStatus{header, headerFileSize, headerLastModifiedTime2}));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, update_file_for_non_existing_entry)
{
    cache.update(header);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, update_file_stats)
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

TEST_F(FileStatusCache, update_files_does_not_change_entry_count)
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

TEST_F(FileStatusCache, update_files_for_non_existing_entry)
{
    cache.update(entries);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, new_modified_entries)
{
    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, entries);
}

TEST_F(FileStatusCache, no_new_modified_entries)
{
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, IsEmpty());
}

TEST_F(FileStatusCache, some_new_modified_entries)
{
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, source2));
}

TEST_F(FileStatusCache, some_already_existing_modified_entries)
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

TEST_F(FileStatusCache, some_already_existing_and_some_new_modified_entries)
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

TEST_F(FileStatusCache, time_is_updated_for_some_already_existing_modified_entries)
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
