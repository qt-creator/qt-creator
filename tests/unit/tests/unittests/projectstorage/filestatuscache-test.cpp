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
    using file_time_type = std::filesystem::file_time_type;

protected:
    FileStatusCache()
    {
        ON_CALL(fileSystem, fileStatus(Eq(header)))
            .WillByDefault(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)));
        ON_CALL(fileSystem, fileStatus(Eq(source)))
            .WillByDefault(Return(createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
        ON_CALL(fileSystem, fileStatus(Eq(directory)))
            .WillByDefault(Return(createFileStatus(directory, 0, directoryLastModifiedTime)));
        ON_CALL(fileSystem, fileStatus(Eq(header2)))
            .WillByDefault(Return(createFileStatus(header2, header2FileSize, header2LastModifiedTime)));
        ON_CALL(fileSystem, fileStatus(Eq(source2)))
            .WillByDefault(Return(createFileStatus(source2, source2FileSize, source2LastModifiedTime)));
        ON_CALL(fileSystem, fileStatus(Eq(directory2)))
            .WillByDefault(Return(createFileStatus(directory2, 0, directory2LastModifiedTime)));
    }

    static QmlDesigner::FileStatus createFileStatus(SourceId sourceId,
                                                    long long size,
                                                    long long modifiedTime)
    {
        using file_time_type = std::filesystem::file_time_type;

        return QmlDesigner::FileStatus{sourceId,
                                       size,
                                       file_time_type{file_time_type::duration{modifiedTime}}};
    }

    static QmlDesigner::FileStatus createFileStatus(SourceId sourceId,
                                                    long long size,
                                                    file_time_type modifiedTime)
    {
        return QmlDesigner::FileStatus{sourceId, size, modifiedTime};
    }

    file_time_type createFileTimeType(long long time)
    {
        return file_time_type{file_time_type::duration{time}};
    }

protected:
    NiceMock<FileSystemMock> fileSystem;
    QmlDesigner::FileStatusCache cache{fileSystem};
    QmlDesigner::FileNameId headerFileNameId = QmlDesigner::FileNameId::create(1);
    QmlDesigner::FileNameId sourceFileNameId = QmlDesigner::FileNameId::create(2);
    QmlDesigner::DirectoryPathId directoryOneId = QmlDesigner::DirectoryPathId::create(1);
    QmlDesigner::DirectoryPathId directoryTwoId = QmlDesigner::DirectoryPathId::create(2);
    SourceId header{SourceId::create(directoryOneId, headerFileNameId)};
    SourceId source{SourceId::create(directoryOneId, sourceFileNameId)};
    SourceId directory{SourceId::create(directoryOneId)};
    SourceId header2{SourceId::create(directoryTwoId, headerFileNameId)};
    SourceId source2{SourceId::create(directoryTwoId, sourceFileNameId)};
    SourceId directory2{SourceId::create(directoryTwoId)};
    SourceIds entries{header, source, header2, source2};
    file_time_type headerLastModifiedTime = createFileTimeType(100);
    file_time_type headerLastModifiedTime2 = createFileTimeType(110);
    long long headerFileSize = 1000;
    long long headerFileSize2 = 1100;
    file_time_type header2LastModifiedTime = createFileTimeType(300);
    file_time_type header2LastModifiedTime2 = createFileTimeType(310);
    long long header2FileSize = 3000;
    long long header2FileSize2 = 3100;
    file_time_type sourceLastModifiedTime = createFileTimeType(200);
    file_time_type source2LastModifiedTime = createFileTimeType(400);
    long long sourceFileSize = 2000;
    long long source2FileSize = 4000;
    file_time_type directoryLastModifiedTime = createFileTimeType(500);
    file_time_type directory2LastModifiedTime = createFileTimeType(600);
};

TEST_F(FileStatusCache, create_entry_with_find)
{
    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, create_entry_with_update_and_find)
{
    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, ask_created_entry_with_find_for_last_modified_time)
{
    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, ask_created_entry_with_update_and_find_for_last_modified_time)
{
    auto fileStatus = cache.updateAndFind(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, find_cached_entry)
{
    cache.find(header);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, find_cached_entry_with_update_and_find)
{
    cache.find(header);

    auto fileStatus = cache.updateAndFind(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, dont_add_entry_twice_with_find)
{
    cache.find(header);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, dont_add_entry_twice_with_update_and_find)
{
    cache.find(header);

    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, add_new_entry_with_find)
{
    cache.find(header);

    cache.find(source);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, add_new_entry_with_update_and_find)
{
    cache.find(header);

    cache.updateAndFind(source);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, ask_new_entry_with_find_for_last_modified_time)
{
    cache.find(header);

    auto fileStatus = cache.find(source);

    ASSERT_THAT(fileStatus, (createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
}

TEST_F(FileStatusCache, ask_new_entry_with_update_and_find_for_last_modified_time)
{
    cache.find(header);

    auto fileStatus = cache.updateAndFind(source);

    ASSERT_THAT(fileStatus, (createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
}

TEST_F(FileStatusCache, add_new_entry_with_find_reverse_order)
{
    cache.find(source);

    cache.find(header);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, add_new_entry_with_update_and_find_reverse_order)
{
    cache.find(source);

    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, ask_new_entry_with_find_reverse_order_added_for_last_modified_time)
{
    cache.find(source);

    auto fileStatus = cache.find(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, ask_new_entry_with_update_and_find_reverse_order_added_for_last_modified_time)
{
    cache.find(source);

    auto fileStatus = cache.updateAndFind(header);

    ASSERT_THAT(fileStatus, (createFileStatus(header, headerFileSize, headerLastModifiedTime)));
}

TEST_F(FileStatusCache, update_and_find_file)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
    cache.find(header);

    auto found = cache.updateAndFind(header);

    ASSERT_THAT(found, (createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
}

TEST_F(FileStatusCache, update_and_find_file_does_not_change_entry_count)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime2)));
    cache.find(header);

    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, update_and_find_file_for_non_existing_entry)
{
    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, update_and_find_file_stats)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
    cache.find(header);

    auto found = cache.updateAndFind(header);

    ASSERT_THAT(found, (createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
}

TEST_F(FileStatusCache, update_and_find_files_does_not_change_entry_count)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize2, headerLastModifiedTime)));
    cache.find(header);

    cache.updateAndFind(header);

    ASSERT_THAT(cache, SizeIs(1));
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
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize2, headerLastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header2, header2FileSize, header2LastModifiedTime)))
        .WillOnce(Return(createFileStatus(header2, header2FileSize2, header2LastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(createFileStatus(source2, source2FileSize, source2LastModifiedTime)));
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2));
}

TEST_F(FileStatusCache, some_already_existing_and_some_new_modified_entries)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .WillRepeatedly(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header2, header2FileSize, header2LastModifiedTime)))
        .WillOnce(Return(createFileStatus(header2, header2FileSize, header2LastModifiedTime2)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .WillRepeatedly(Return(createFileStatus(source2, source2FileSize, source2LastModifiedTime)));
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2, source2));
}

TEST_F(FileStatusCache, time_is_updated_for_some_already_existing_modified_entries)
{
    EXPECT_CALL(fileSystem, fileStatus(Eq(header)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header, headerFileSize, headerLastModifiedTime)))
        .WillOnce(Return(createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(header2)))
        .Times(2)
        .WillOnce(Return(createFileStatus(header2, header2FileSize, header2LastModifiedTime)))
        .WillOnce(Return(createFileStatus(header2, header2FileSize2, header2LastModifiedTime2)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source)))
        .Times(2)
        .WillRepeatedly(Return(createFileStatus(source, sourceFileSize, sourceLastModifiedTime)));
    EXPECT_CALL(fileSystem, fileStatus(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(createFileStatus(source2, source2FileSize, source2LastModifiedTime)));
    cache.modified(entries);

    cache.modified(entries);

    ASSERT_THAT(cache.find(header),
                (createFileStatus(header, headerFileSize2, headerLastModifiedTime2)));
}

TEST_F(FileStatusCache, remove_entry_with_directory_id)
{
    cache.find(header);
    cache.find(source);
    cache.find(directory);
    cache.find(header2);
    cache.find(directory2);

    cache.remove({header.directoryPathId()});

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, remove_entry_with_source_idss)
{
    cache.find(header);
    cache.find(source);
    cache.find(directory);
    cache.find(header2);
    cache.find(directory2);

    cache.remove({header, source});

    ASSERT_THAT(cache, SizeIs(3));
}

} // namespace
