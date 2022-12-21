// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "projectstoragemock.h"
#include "sqlitedatabasemock.h"

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

TEST_F(SourcePathCache, SourceIdWithOutAnyEntryCallSourceContextId)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceIdWithOutAnyEntryCalls)
{
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceIdOfSourceIdWithOutAnyEntry)
{
    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, SourceIdWithSourceContextIdAndSourceName)
{
    auto sourceContextId = cache.sourceContextId("/path/to"_sv);

    auto sourceId = cache.sourceId(sourceContextId, "file.cpp"_sv);

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, IfEntryExistsDontCallInStrorage)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp"))).Times(0);

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, IfDirectoryEntryExistsDontCallFetchSourceContextIdButStillCallFetchSourceId)
{
    cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, GetSourceIdWithCachedValue)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(42));
}

TEST_F(SourcePathCache, GetSourceIdWithSourceContextIdCached)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceId, SourceId::create(63));
}

TEST_F(SourcePathCache, ThrowForGettingAFilePathWithAnInvalidId)
{
    SourceId sourceId;

    ASSERT_THROW(cache.sourcePath(sourceId), QmlDesigner::NoSourcePathForInvalidSourceId);
}

TEST_F(SourcePathCache, GetAFilePath)
{
    SourceId sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourcePath = cache.sourcePath(sourceId);

    ASSERT_THAT(sourcePath, Eq(SourcePathView{"/path/to/file.cpp"}));
}

TEST_F(SourcePathCache, GetAFilePathWithCachedSourceId)
{
    SourceId sourceId{SourceId::create(42)};

    auto sourcePath = cache.sourcePath(sourceId);

    ASSERT_THAT(sourcePath, Eq(SourcePathView{"/path/to/file.cpp"}));
}

TEST_F(SourcePathCache, FileNamesAreUniqueForEveryDirectory)
{
    SourceId sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    SourceId sourcePath2Id = cache.sourceId(SourcePathView("/path2/to/file.cpp"));

    ASSERT_THAT(sourcePath2Id, Ne(sourceId));
}

TEST_F(SourcePathCache, DuplicateFilePathsAreEqual)
{
    SourceId sourcePath1Id = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    SourceId sourcePath2Id = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourcePath2Id, Eq(sourcePath1Id));
}

TEST_F(SourcePathCache, SourceContextIdCallsFetchSourceContextId)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, SecondSourceContextIdCallsNotFetchSourceContextId)
{
    cache.sourceContextId(Utils::SmallString("/path/to"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);

    cache.sourceContextId(Utils::SmallString("/path/to"));
}

TEST_F(SourcePathCache, SourceContextIdWithTrailingSlash)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextId(Utils::SmallString("/path/to/"));
}

TEST_F(SourcePathCache, SourceContextId)
{
    auto id = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, SourceContextIdIsAlreadyInCache)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, SourceContextIdIsAlreadyInCacheWithTrailingSlash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, SourceContextIdIsAlreadyInCacheWithAndWithoutTrailingSlash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, SourceContextIdIsAlreadyInCacheWithoutAndWithTrailingSlash)
{
    auto firstId = cache.sourceContextId(Utils::SmallString("/path/to"));

    auto secondId = cache.sourceContextId(Utils::SmallString("/path/to/"));

    ASSERT_THAT(secondId, firstId);
}

TEST_F(SourcePathCache, ThrowForGettingADirectoryPathWithAnInvalidId)
{
    SourceContextId sourceContextId;

    ASSERT_THROW(cache.sourceContextPath(sourceContextId),
                 QmlDesigner::NoSourceContextPathForInvalidSourceContextId);
}

TEST_F(SourcePathCache, GetADirectoryPath)
{
    SourceContextId sourceContextId{SourceContextId::create(5)};

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, GetADirectoryPathWithCachedSourceContextId)
{
    SourceContextId sourceContextId{SourceContextId::create(5)};
    cache.sourceContextPath(sourceContextId);

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, DirectoryPathCallsFetchDirectoryPath)
{
    EXPECT_CALL(storageMock, fetchSourceContextPath(Eq(SourceContextId::create(5))));

    cache.sourceContextPath(SourceContextId::create(5));
}

TEST_F(SourcePathCache, SecondDirectoryPathCallsNotFetchDirectoryPath)
{
    cache.sourceContextPath(SourceContextId::create(5));

    EXPECT_CALL(storageMock, fetchSourceContextPath(_)).Times(0);

    cache.sourceContextPath(SourceContextId::create(5));
}

TEST_F(SourcePathCache, ThrowForGettingASourceContextIdWithAnInvalidSourceId)
{
    SourceId sourceId;

    ASSERT_THROW(cache.sourceContextId(sourceId), QmlDesigner::NoSourcePathForInvalidSourceId);
}

TEST_F(SourcePathCache, FetchSourceContextIdBySourceId)
{
    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, FetchSourceContextIdBySourceIdCached)
{
    cache.sourceContextId(SourceId::create(42));

    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, FetchFilePathAfterFetchingSourceContextIdBySourceId)
{
    cache.sourceContextId(SourceId::create(42));

    auto sourcePath = cache.sourcePath(SourceId::create(42));

    ASSERT_THAT(sourcePath, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, FetchSourceContextIdAfterFetchingFilePathBySourceId)
{
    cache.sourcePath(SourceId::create(42));

    auto sourceContextId = cache.sourceContextId(SourceId::create(42));

    ASSERT_THAT(sourceContextId, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, FetchAllSourceContextsAndSourcesAtCreation)
{
    EXPECT_CALL(storageMock, fetchAllSourceContexts());
    EXPECT_CALL(storageMock, fetchAllSources());

    Cache cache{storageMock};
}

TEST_F(SourcePathCache, GetFileIdInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(SourceId::create(72)));
}

TEST_F(SourcePathCache, GetSourceContextIdInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceContextId(SourceId::create(42));

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, GetDirectoryPathInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourceContextPath(SourceContextId::create(5));

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, GetFilePathInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourcePath(SourceId::create(42));

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, GetFileIdInAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(SourceId::create(72)));
}

TEST_F(SourcePathCache, DontPopulateIfNotEmpty)
{
    cacheNotFilled.sourceId("/path/to/file.cpp");

    EXPECT_CALL(storageMockFilled, fetchAllSourceContexts()).Times(0);
    EXPECT_CALL(storageMockFilled, fetchAllSources()).Times(0);

    cacheNotFilled.populateIfEmpty();
}

TEST_F(SourcePathCache, GetSourceContextIdAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceContextId(SourceId::create(42));

    ASSERT_THAT(id, Eq(SourceContextId::create(5)));
}

TEST_F(SourcePathCache, GetDirectoryPathAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourceContextPath(SourceContextId::create(5));

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, GetFilePathAfterPopulateIfEmptye)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourcePath(SourceId::create(42));

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceContextAndSourceIdWithOutAnyEntryCallSourceContextId)
{
    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceContextAndSourceIdWithOutAnyEntryCalls)
{
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceContextAndSourceIdOfSourceIdWithOutAnyEntry)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Pair(SourceContextId::create(5), SourceId::create(42)));
}

TEST_F(SourcePathCache, SourceContextAndSourceIdIfEntryExistsDontCallInStrorage)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp"))).Times(0);

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache,
       SourceContextAndSourceIdIfDirectoryEntryExistsDontCallFetchSourceContextIdButStillCallFetchSourceId)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId::create(5), Eq("file.cpp")));

    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceContextAndSourceIdGetSourceIdWithCachedValue)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, Pair(SourceContextId::create(5), SourceId::create(42)));
}

TEST_F(SourcePathCache, GetSourceContextAndSourceIdWithSourceContextIdCached)
{
    cache.sourceContextAndSourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Pair(SourceContextId::create(5), SourceId::create(63)));
}

TEST_F(SourcePathCache, GetSourceContextAndSourceIdFileNamesAreUniqueForEveryDirectory)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId2 = cache.sourceContextAndSourceId(
        SourcePathView("/path2/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Ne(sourceContextAndSourceId2));
}

TEST_F(SourcePathCache, GetSourceContextAndSourceIdDuplicateFilePathsAreEqual)
{
    auto sourceContextAndSourceId = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    auto sourceContextAndSourceId2 = cache.sourceContextAndSourceId(
        SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceContextAndSourceId, Eq(sourceContextAndSourceId2));
}

} // namespace
