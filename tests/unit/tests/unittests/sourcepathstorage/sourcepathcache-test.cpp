// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/sqlitedatabasemock.h"

#include <sourcepathstorage/sourcepathcache.h>

namespace {

using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::SourceNameId;
using Cache = QmlDesigner::SourcePathCache<NiceMock<ProjectStorageMock>>;
using NFP = QmlDesigner::SourcePath;
using QmlDesigner::Cache::SourceName;
using QmlDesigner::SourcePathView;
using QmlDesigner::SourcePathViews;

class SourcePathCache : public testing::Test
{
protected:
    SourcePathCache()
    {
        ON_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).WillByDefault(Return(sourceContextId5));
        ON_CALL(storageMock, fetchSourceContextId(Eq("/path2/to"))).WillByDefault(Return(sourceContextId6));
        ON_CALL(storageMock, fetchSourceNameId(Eq("file.cpp"))).WillByDefault(Return(sourceNameId42));
        ON_CALL(storageMock, fetchSourceNameId(Eq("file2.cpp"))).WillByDefault(Return(sourceNameId63));
        ON_CALL(storageMock, fetchSourceContextPath(sourceContextId5))
            .WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(storageMock, fetchSourceName(sourceNameId42)).WillByDefault(Return("file.cpp"));
        ON_CALL(storageMockFilled, fetchAllSourceNames())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::SourceName>(
                {{"file.cpp", sourceNameId42}, {"file2.cpp", sourceNameId63}})));
        ON_CALL(storageMockFilled, fetchAllSourceContexts())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::SourceContext>(
                {{"/path2/to", sourceContextId6}, {"/path/to", sourceContextId5}})));
        ON_CALL(storageMockFilled, fetchSourceContextId(Eq("/path/to")))
            .WillByDefault(Return(sourceContextId5));
        ON_CALL(storageMockFilled, fetchSourceNameId(Eq("file.cpp"))).WillByDefault(Return(sourceNameId42));
    }

protected:
    SourceNameId sourceNameId42 = SourceNameId::create(42);
    SourceNameId sourceNameId63 = SourceNameId::create(63);
    SourceContextId sourceContextId5 = SourceContextId::create(5);
    SourceContextId sourceContextId6 = SourceContextId::create(6);
    SourceId sourceId542 = SourceId::create(sourceNameId42, sourceContextId5);
    SourceId sourceId563 = SourceId::create(sourceNameId63, sourceContextId5);
    SourceId sourceId642 = SourceId::create(sourceNameId42, sourceContextId6);
    NiceMock<ProjectStorageMock> storageMock;
    Cache cache{storageMock};
    NiceMock<ProjectStorageMock> storageMockFilled;
    Cache cacheNotFilled{storageMockFilled};
};

TEST_F(SourcePathCache, source_id_with_out_any_entry_call_source_context_id)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_id_with_out_any_entry_calls)
{
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_id_of_source_id_with_out_any_entry)
{
    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, source_id_with_source_context_id_and_source_name)
{
    auto sourceContextId = cache.sourceContextId("/path/to"_sv);

    auto sourceId = cache.sourceId(sourceContextId, "file.cpp"_sv);

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, if_entry_exists_dont_call_in_strorage)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp"))).Times(0);

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, if_directory_entry_exists_dont_call_fetch_source_context_id_but_still_call_fetch_source_id)
{
    cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, get_source_id_with_cached_value)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, sourceId542);
}

TEST_F(SourcePathCache, get_source_id_with_source_context_id_cached)
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

TEST_F(SourcePathCache, source_context_id_calls_fetch_source_context_id)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, second_source_context_id_calls_not_fetch_source_context_id)
{
    cache.sourceContextId(Utils::SmallString("/path/to"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);

    cache.sourceContextId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, source_context_id_with_trailing_slash)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextId(Utils::SmallString("/path/to/"));
}

TEST_F(SourcePathCache, source_context_id)
{
    auto id = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(id, Eq(sourceContextId5));
}

TEST_F(SourcePathCache, source_context_id_is_already_in_cache)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, source_context_id_is_already_in_cache_with_trailing_slash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, source_context_id_is_already_in_cache_with_and_without_trailing_slash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, source_context_id_is_already_in_cache_without_and_with_trailing_slash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, throw_for_getting_a_directory_path_with_an_invalid_id)
{
    SourceContextId sourceContextId;

    ASSERT_THROW(cache.sourceContextPath(sourceContextId),
                 QmlDesigner::NoSourceContextPathForInvalidSourceContextId);
}

TEST_F(SourcePathCache, get_a_directory_path)
{
    SourceContextId sourceContextId{sourceContextId5};

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, get_a_directory_path_with_cached_source_context_id)
{
    SourceContextId sourceContextId{sourceContextId5};
    cache.sourceContextPath(sourceContextId);

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, directory_path_calls_fetch_directory_path)
{
    EXPECT_CALL(storageMock, fetchSourceContextPath(Eq(sourceContextId5)));

    cache.sourceContextPath(sourceContextId5);
}

TEST_F(SourcePathCache, second_directory_path_calls_not_fetch_directory_path)
{
    cache.sourceContextPath(sourceContextId5);

    EXPECT_CALL(storageMock, fetchSourceContextPath(_)).Times(0);

    cache.sourceContextPath(sourceContextId5);
}

TEST_F(SourcePathCache, fetch_source_context_from_source_id)
{
    auto sourceContextId = sourceId542.contextId();

    ASSERT_THAT(sourceContextId, Eq(sourceContextId5));
}

TEST_F(SourcePathCache, fetch_source_context_id_after_fetching_file_path_by_source_id)
{
    cache.sourcePath(sourceId542);

    auto sourceContextId = sourceId542.contextId();

    ASSERT_THAT(sourceContextId, Eq(sourceContextId5));
}

TEST_F(SourcePathCache, fetch_all_source_contexts_and_sources_at_creation)
{
    EXPECT_CALL(storageMock, fetchAllSourceContexts());
    EXPECT_CALL(storageMock, fetchAllSourceNames());

    Cache cache{storageMock};
}

TEST_F(SourcePathCache, get_file_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(sourceId642));
}

TEST_F(SourcePathCache, get_source_context_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = sourceId542.contextId();

    ASSERT_THAT(id, Eq(sourceContextId5));
}

TEST_F(SourcePathCache, get_directory_path_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourceContextPath(sourceContextId5);

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

    EXPECT_CALL(storageMockFilled, fetchAllSourceContexts()).Times(0);
    EXPECT_CALL(storageMockFilled, fetchAllSourceNames()).Times(0);

    cacheNotFilled.populateIfEmpty();
}

TEST_F(SourcePathCache, get_directory_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourceContextPath(sourceContextId5);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, get_file_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourcePath(sourceId542);

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_with_out_any_entry_call_source_context_id)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_with_out_any_entry_calls)
{
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_of_source_id_with_out_any_entry)
{
    SourcePathView path("/path/to/file.cpp");

    auto sourceId = cache.sourceId(path);

    ASSERT_THAT(sourceId.contextId(), sourceContextId5);
}

TEST_F(SourcePathCache, source_context_and_source_id_if_entry_exists_dont_call_in_strorage)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp"))).Times(0);

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache,
       source_context_and_source_id_if_directory_entry_exists_dont_call_fetch_source_context_id_but_still_call_fetch_source_id)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceNameId(Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_get_source_id_with_cached_value)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, Pair(sourceContextId5, sourceId542));
}

TEST_F(SourcePathCache, get_source_context_and_source_id_with_source_context_id_cached)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Pair(sourceContextId5, sourceId563));
}

TEST_F(SourcePathCache, get_source_context_and_source_id_file_names_are_unique_for_every_directory)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId2 = cache.sourceContextAndSourceId(
        SourcePathView("/path2/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Ne(sourceContextAndSourceId2));
}

TEST_F(SourcePathCache, get_source_context_and_source_id_duplicate_file_paths_are_equal)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId2 = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Eq(sourceContextAndSourceId2));
}

} // namespace
