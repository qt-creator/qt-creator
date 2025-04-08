// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/sqlitedatabasemock.h"

#include <sourcepathstorage/sourcepathcache.h>

namespace {

using QmlDesigner::DirectoryPathId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::FileNameId;
using Cache = QmlDesigner::SourcePathCache<NiceMock<ProjectStorageMock>>;
using NFP = QmlDesigner::SourcePath;
using QmlDesigner::Cache::FileName;
using QmlDesigner::SourcePathView;
using QmlDesigner::SourcePathViews;

class SourcePathCache : public testing::Test
{
protected:
    SourcePathCache()
    {
        ON_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to"))).WillByDefault(Return(directoryPathId5));
        ON_CALL(storageMock, fetchDirectoryPathId(Eq("/path2/to"))).WillByDefault(Return(directoryPathId6));
        ON_CALL(storageMock, fetchFileNameId(Eq("file.cpp"))).WillByDefault(Return(fileNameId42));
        ON_CALL(storageMock, fetchFileNameId(Eq("file2.cpp"))).WillByDefault(Return(fileNameId63));
        ON_CALL(storageMock, fetchDirectoryPath(directoryPathId5))
            .WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(storageMock, fetchFileName(fileNameId42)).WillByDefault(Return("file.cpp"));
        ON_CALL(storageMockFilled, fetchAllFileNames())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::FileName>(
                {{"file.cpp", fileNameId42}, {"file2.cpp", fileNameId63}})));
        ON_CALL(storageMockFilled, fetchAllDirectoryPaths())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::DirectoryPath>(
                {{"/path2/to", directoryPathId6}, {"/path/to", directoryPathId5}})));
        ON_CALL(storageMockFilled, fetchDirectoryPathId(Eq("/path/to")))
            .WillByDefault(Return(directoryPathId5));
        ON_CALL(storageMockFilled, fetchFileNameId(Eq("file.cpp"))).WillByDefault(Return(fileNameId42));
    }

protected:
    FileNameId fileNameId42 = FileNameId::create(42);
    FileNameId fileNameId63 = FileNameId::create(63);
    DirectoryPathId directoryPathId5 = DirectoryPathId::create(5);
    DirectoryPathId directoryPathId6 = DirectoryPathId::create(6);
    SourceId sourceId542 = SourceId::create(directoryPathId5, fileNameId42);
    SourceId sourceId563 = SourceId::create(directoryPathId5, fileNameId63);
    SourceId sourceId642 = SourceId::create(directoryPathId6, fileNameId42);
    NiceMock<ProjectStorageMock> storageMock;
    Cache cache{storageMock};
    NiceMock<ProjectStorageMock> storageMockFilled;
    Cache cacheNotFilled{storageMockFilled};
};

TEST_F(SourcePathCache, source_id_with_out_any_entry_call_directory_path_id)
{
    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_id_with_out_any_entry_calls)
{
    EXPECT_CALL(storageMock, fetchFileNameId(Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_id_of_source_id_with_out_any_entry)
{
    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, source_id_with_directory_path_id_and_file_name)
{
    auto directoryPathId = cache.directoryPathId("/path/to"_sv);

    auto sourceId = cache.sourceId(directoryPathId, "file.cpp"_sv);

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, if_entry_exists_dont_call_in_strorage)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchFileNameId(Eq("file.cpp"))).Times(0);

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache,
       if_directory_entry_exists_dont_call_fetch_directory_path_id_but_still_call_fetch_source_id)
{
    cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchFileNameId(Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, get_source_id_with_cached_value)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, get_source_id_with_directory_path_id_cached)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceId, sourceId563);
}

TEST_F(SourcePathCache, throw_for_getting_a_file_path_with_an_invalid_id)
{
    SourceId sourceId;

    ASSERT_THROW(cache.sourcePath(sourceId), QmlDesigner::NoSourcePathForInvalidSourceId);
}

TEST_F(SourcePathCache, get_a_file_path)
{
    SourceId sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourcePath = cache.sourcePath(sourceId);

    ASSERT_THAT(sourcePath, Eq(SourcePathView{"/path/to/file.cpp"}));
}

TEST_F(SourcePathCache, get_a_file_path_with_cached_source_id)
{
    auto sourcePath = cache.sourcePath(sourceId542);

    ASSERT_THAT(sourcePath, Eq(SourcePathView{"/path/to/file.cpp"}));
}

TEST_F(SourcePathCache, file_names_are_unique_for_every_directory)
{
    SourceId sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    SourceId sourcePath2Id = cache.sourceId(SourcePathView("/path2/to/file.cpp"));

    ASSERT_THAT(sourcePath2Id, Ne(sourceId));
}

TEST_F(SourcePathCache, duplicate_file_paths_are_equal)
{
    SourceId sourcePath1Id = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    SourceId sourcePath2Id = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourcePath2Id, Eq(sourcePath1Id));
}

TEST_F(SourcePathCache, directory_path_id_calls_fetch_directory_path_id)
{
    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to")));

    cache.directoryPathId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, second_directory_path_id_calls_not_fetch_directory_path_id)
{
    cache.directoryPathId(Utils::SmallString("/path/to"));

    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to"))).Times(0);

    cache.directoryPathId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, directory_path_id_with_trailing_slash)
{
    EXPECT_CALL(storageMock, fetchDirectoryPathId(Eq("/path/to")));

    cache.directoryPathId(Utils::SmallString("/path/to/"));
}

TEST_F(SourcePathCache, directory_path_id)
{
    auto id = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(id, Eq(directoryPathId5));
}

TEST_F(SourcePathCache, directory_path_id_is_already_in_cache)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, directory_path_id_is_already_in_cache_with_trailing_slash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, directory_path_id_is_already_in_cache_with_and_without_trailing_slash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, directory_path_id_is_already_in_cache_without_and_with_trailing_slash)
{
    auto firstId = cache.directoryPathId(Utils::SmallString("/path/to"));

    auto secondId = cache.directoryPathId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, throw_for_getting_a_directory_path_with_an_invalid_id)
{
    DirectoryPathId directoryPathId;

    ASSERT_THROW(cache.directoryPath(directoryPathId),
                 QmlDesigner::NoDirectoryPathForInvalidDirectoryPathId);
}

TEST_F(SourcePathCache, get_a_directory_path)
{
    DirectoryPathId directoryPathId{directoryPathId5};

    auto directoryPath = cache.directoryPath(directoryPathId);

    ASSERT_THAT(directoryPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, get_a_directory_path_with_cached_directory_path_id)
{
    DirectoryPathId directoryPathId{directoryPathId5};
    cache.directoryPath(directoryPathId);

    auto directoryPath = cache.directoryPath(directoryPathId);

    ASSERT_THAT(directoryPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, directory_path_calls_fetch_directory_path)
{
    EXPECT_CALL(storageMock, fetchDirectoryPath(Eq(directoryPathId5)));

    cache.directoryPath(directoryPathId5);
}

TEST_F(SourcePathCache, second_directory_path_calls_not_fetch_directory_path)
{
    cache.directoryPath(directoryPathId5);

    EXPECT_CALL(storageMock, fetchDirectoryPath(_)).Times(0);

    cache.directoryPath(directoryPathId5);
}

TEST_F(SourcePathCache, fetch_directory_path_from_source_id)
{
    auto directoryPathId = sourceId542.directoryPathId();

    ASSERT_THAT(directoryPathId, Eq(directoryPathId5));
}

TEST_F(SourcePathCache, fetch_directory_path_id_after_fetching_file_path_by_source_id)
{
    cache.sourcePath(sourceId542);

    auto directoryPathId = sourceId542.directoryPathId();

    ASSERT_THAT(directoryPathId, Eq(directoryPathId5));
}

TEST_F(SourcePathCache, fetch_all_directory_paths_and_sources_at_creation)
{
    EXPECT_CALL(storageMock, fetchAllDirectoryPaths());
    EXPECT_CALL(storageMock, fetchAllFileNames());

    Cache cache{storageMock};
}

TEST_F(SourcePathCache, get_file_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(sourceId642));
}

TEST_F(SourcePathCache, get_directory_path_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = sourceId542.directoryPathId();

    ASSERT_THAT(id, Eq(directoryPathId5));
}

TEST_F(SourcePathCache, get_directory_path_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.directoryPath(directoryPathId5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, get_file_path_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourcePath(sourceId542);

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, get_file_id_in_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(sourceId642));
}

TEST_F(SourcePathCache, dont_populate_if_not_empty)
{
    cacheNotFilled.sourceId("/path/to/file.cpp");

    EXPECT_CALL(storageMockFilled, fetchAllDirectoryPaths()).Times(0);
    EXPECT_CALL(storageMockFilled, fetchAllFileNames()).Times(0);

    cacheNotFilled.populateIfEmpty();
}

TEST_F(SourcePathCache, get_directory_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.directoryPath(directoryPathId5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, get_file_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourcePath(sourceId542);

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, file_name_id_calls_fetch_file_name_id)
{
    EXPECT_CALL(storageMock, fetchFileNameId(Eq("file.cpp")));

    cache.fileNameId(Utils::SmallString("file.cpp"));
}

TEST_F(SourcePathCache, second_file_name_id_calls_not_fetch_file_name_id)
{
    cache.fileNameId(Utils::SmallString("file.cpp"));

    EXPECT_CALL(storageMock, fetchFileNameId(Eq("file.cpp"))).Times(0);

    cache.fileNameId(Utils::SmallString("file.cpp"));
}

TEST_F(SourcePathCache, file_name_id)
{
    auto id = cache.fileNameId(Utils::SmallString("file.cpp"));

    ASSERT_THAT(id, Eq(fileNameId42));
}

TEST_F(SourcePathCache, file_name_id_is_already_in_cache)
{
    auto firstId = cache.fileNameId(Utils::SmallString("file.cpp"));

    auto secondId = cache.fileNameId(Utils::SmallString("file.cpp"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, throw_for_getting_file_name_with_an_invalid_id)
{
    FileNameId fileNameId;

    ASSERT_THROW(cache.fileName(fileNameId), QmlDesigner::NoFileNameForInvalidFileNameId);
}

TEST_F(SourcePathCache, get_a_file_name)
{
    FileNameId fileNameId{fileNameId42};

    auto fileName = cache.fileName(fileNameId);

    ASSERT_THAT(fileName, Eq(Utils::SmallStringView{"file.cpp"}));
}

TEST_F(SourcePathCache, get_a_file_name_with_cached_file_name_id)
{
    FileNameId fileNameId{fileNameId42};
    cache.fileName(fileNameId);

    auto fileName = cache.fileName(fileNameId);

    ASSERT_THAT(fileName, Eq(Utils::SmallStringView{"file.cpp"}));
}

TEST_F(SourcePathCache, file_name_calls_fetch_file_name)
{
    EXPECT_CALL(storageMock, fetchFileName(Eq(fileNameId42)));

    cache.fileName(fileNameId42);
}

TEST_F(SourcePathCache, second_file_name_calls_not_fetch_file_name)
{
    cache.fileName(fileNameId42);

    EXPECT_CALL(storageMock, fetchFileName(_)).Times(0);

    cache.fileName(fileNameId42);
}

} // namespace
