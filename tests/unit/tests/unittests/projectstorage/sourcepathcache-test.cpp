// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/sqlitedatabasemock.h"

#include <projectstorage/sourcepathcache.h>

namespace {

using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using Cache = QmlDesigner::SourcePathCache<NiceMock<ProjectStorageMock>>;
using NFP = QmlDesigner::SourcePath;
using QmlDesigner::SourcePathView;
using QmlDesigner::SourcePathViews;
using QmlDesigner::Cache::SourceNameAndSourceContextId;

class SourcePathCache : public testing::Test
{
protected:
    SourcePathCache()
    {
        ON_CALL(storageMock, fetchSourceContextId(Eq("/path/to")))
            .WillByDefault(Return(SourceContextId::create(5)));
        ON_CALL(storageMock, fetchSourceContextId(Eq("/path2/to")))
            .WillByDefault(Return(SourceContextId::create(6)));
        ON_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")))
            .WillByDefault(Return(SourceId::create(42)));
        ON_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file2.cpp")))
            .WillByDefault(Return(SourceId::create(63)));
        ON_CALL(storageMock, fetchSourceId(SourceContextId::create(6), Eq("file.cpp")))
            .WillByDefault(Return(SourceId::create(72)));
        ON_CALL(storageMock, fetchSourceContextPath(SourceContextId::create(5)))
            .WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(storageMock, fetchSourceNameAndSourceContextId(SourceId::create(42)))
            .WillByDefault(
                Return(SourceNameAndSourceContextId("file.cpp", SourceContextId::create(5))));
        ON_CALL(storageMockFilled, fetchAllSources())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::Source>({
                {"file.cpp", SourceContextId::create(6), SourceId::create(72)},
                {"file2.cpp", SourceContextId::create(5), SourceId::create(63)},
                {"file.cpp", SourceContextId::create(5), SourceId::create(42)},
            })));
        ON_CALL(storageMockFilled, fetchAllSourceContexts())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::SourceContext>(
                {{"/path2/to", SourceContextId::create(6)}, {"/path/to", SourceContextId::create(5)}})));
        ON_CALL(storageMockFilled, fetchSourceContextId(Eq("/path/to")))
            .WillByDefault(Return(SourceContextId::create(5)));
        ON_CALL(storageMockFilled, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")))
            .WillByDefault(Return(SourceId::create(42)));
    }

protected:
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
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_id_of_source_id_with_out_any_entry)
{
    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, source_id_with_source_context_id_and_source_name)
{
    auto sourceContextId = cache.sourceContextId("/path/to"_sv);

    auto sourceId = cache.sourceId(sourceContextId, "file.cpp"_sv);

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, if_entry_exists_dont_call_in_strorage)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp"))).Times(0);

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, if_directory_entry_exists_dont_call_fetch_source_context_id_but_still_call_fetch_source_id)
{
    cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, get_source_id_with_cached_value)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, get_source_id_with_source_context_id_cached)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(63));
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
    SourceId sourceId{SourceId::create(42)};

    auto sourcePath = cache.sourcePath(sourceId);

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

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
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
    SourceContextId sourceContextId{SourceContextId::create(5)};

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, get_a_directory_path_with_cached_source_context_id)
{
    SourceContextId sourceContextId{SourceContextId::create(5)};
    cache.sourceContextPath(sourceContextId);

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, directory_path_calls_fetch_directory_path)
{
    EXPECT_CALL(storageMock, fetchSourceContextPath(Eq(SourceContextId::create(5))));

    cache.sourceContextPath(SourceContextId::create(5));
}

TEST_F(SourcePathCache, second_directory_path_calls_not_fetch_directory_path)
{
    cache.sourceContextPath(SourceContextId::create(5));

    EXPECT_CALL(storageMock, fetchSourceContextPath(_)).Times(0);

    cache.sourceContextPath(SourceContextId::create(5));
}

TEST_F(SourcePathCache, throw_for_getting_a_source_context_id_with_an_invalid_source_id)
{
    SourceId sourceId;

    ASSERT_THROW(cache.sourceContextId(sourceId), QmlDesigner::NoSourcePathForInvalidSourceId);
}

TEST_F(SourcePathCache, fetch_source_context_id_by_source_id)
{
    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, fetch_source_context_id_by_source_id_cached)
{
    cache.sourceContextId(SourceId::create(42));

    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, fetch_file_path_after_fetching_source_context_id_by_source_id)
{
    cache.sourceContextId(SourceId::create(42));

    auto sourcePath = cache.sourcePath(SourceId::create(42));

    ASSERT_THAT(sourcePath, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, fetch_source_context_id_after_fetching_file_path_by_source_id)
{
    cache.sourcePath(SourceId::create(42));

    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, fetch_all_source_contexts_and_sources_at_creation)
{
    EXPECT_CALL(storageMock, fetchAllSourceContexts());
    EXPECT_CALL(storageMock, fetchAllSources());

    Cache cache{storageMock};
}

TEST_F(SourcePathCache, get_file_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(SourceId::create(72)));
}

TEST_F(SourcePathCache, get_source_context_id_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceContextId(SourceId::create(42));

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, get_directory_path_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourceContextPath(SourceContextId::create(5));

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, get_file_path_in_filled_cache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourcePath(SourceId::create(42));

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, get_file_id_in_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(SourceId::create(72)));
}

TEST_F(SourcePathCache, dont_populate_if_not_empty)
{
    cacheNotFilled.sourceId("/path/to/file.cpp");

    EXPECT_CALL(storageMockFilled, fetchAllSourceContexts()).Times(0);
    EXPECT_CALL(storageMockFilled, fetchAllSources()).Times(0);

    cacheNotFilled.populateIfEmpty();
}

TEST_F(SourcePathCache, get_source_context_id_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceContextId(SourceId::create(42));

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, get_directory_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourceContextPath(SourceContextId::create(5));

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, get_file_path_after_populate_if_empty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourcePath(SourceId::create(42));

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_with_out_any_entry_call_source_context_id)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_with_out_any_entry_calls)
{
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_of_source_id_with_out_any_entry)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Pair(SourceContextId::create(5), SourceId::create(42)));
}

TEST_F(SourcePathCache, source_context_and_source_id_if_entry_exists_dont_call_in_strorage)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp"))).Times(0);

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache,
       source_context_and_source_id_if_directory_entry_exists_dont_call_fetch_source_context_id_but_still_call_fetch_source_id)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, source_context_and_source_id_get_source_id_with_cached_value)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, Pair(SourceContextId::create(5), SourceId::create(42)));
}

TEST_F(SourcePathCache, get_source_context_and_source_id_with_source_context_id_cached)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Pair(SourceContextId::create(5), SourceId::create(63)));
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
