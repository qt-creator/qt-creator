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
            .WillByDefault(Return(SourceContextId{5}));
        ON_CALL(storageMock, fetchSourceContextId(Eq("/path2/to")))
            .WillByDefault(Return(SourceContextId{6}));
        ON_CALL(storageMock, fetchSourceId(SourceContextId{5}, Eq("file.cpp")))
            .WillByDefault(Return(SourceId{42}));
        ON_CALL(storageMock, fetchSourceId(SourceContextId{5}, Eq("file2.cpp")))
            .WillByDefault(Return(SourceId{63}));
        ON_CALL(storageMock, fetchSourceId(SourceContextId{6}, Eq("file.cpp")))
            .WillByDefault(Return(SourceId{72}));
        ON_CALL(storageMock, fetchSourceContextPath(SourceContextId{5}))
            .WillByDefault(Return(Utils::PathString("/path/to")));
        ON_CALL(storageMock, fetchSourceNameAndSourceContextId(SourceId{42}))
            .WillByDefault(Return(SourceNameAndSourceContextId("file.cpp", SourceContextId{5})));
        ON_CALL(storageMockFilled, fetchAllSources())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::Source>({
                {"file.cpp", SourceContextId{6}, SourceId{72}},
                {"file2.cpp", SourceContextId{5}, SourceId{63}},
                {"file.cpp", SourceContextId{5}, SourceId{42}},
            })));
        ON_CALL(storageMockFilled, fetchAllSourceContexts())
            .WillByDefault(Return(std::vector<QmlDesigner::Cache::SourceContext>(
                {{"/path2/to", SourceContextId{6}}, {"/path/to", SourceContextId{5}}})));
        ON_CALL(storageMockFilled, fetchSourceContextId(Eq("/path/to")))
            .WillByDefault(Return(SourceContextId{5}));
        ON_CALL(storageMockFilled, fetchSourceId(SourceContextId{5}, Eq("file.cpp")))
            .WillByDefault(Return(SourceId{42}));
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
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId{5}, Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, SourceIdOfSourceIdWithOutAnyEntry)
{
    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId{42});
}

TEST_F(SourcePathCache, SourceIdWithSourceContextIdAndSourceName)
{
    auto sourceContextId = cache.sourceContextId("/path/to"_sv);

    auto sourceId = cache.sourceId(sourceContextId, "file.cpp"_sv);

    ASSERT_THAT(sourceId, SourceId{42});
}

TEST_F(SourcePathCache, IfEntryExistsDontCallInStrorage)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId{5}, Eq("file.cpp"))).Times(0);

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, IfDirectoryEntryExistsDontCallFetchSourceContextIdButStillCallFetchSourceId)
{
    cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    EXPECT_CALL(storageMock, fetchSourceContextId(Eq("/path/to"))).Times(0);
    EXPECT_CALL(storageMock, fetchSourceId(SourceContextId{5}, Eq("file.cpp")));

    cache.sourceId(SourcePathView("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, GetSourceIdWithCachedValue)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file.cpp"));

    ASSERT_THAT(sourceId, SourceId{42});
}

TEST_F(SourcePathCache, GetSourceIdWithSourceContextIdCached)
{
    cache.sourceId(SourcePathView("/path/to/file.cpp"));

    auto sourceId = cache.sourceId(SourcePathView("/path/to/file2.cpp"));

    ASSERT_THAT(sourceId, SourceId{63});
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
    SourceId sourceId{42};

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

    ASSERT_THAT(id, Eq(SourceContextId{5}));
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
    SourceContextId sourceContextId{5};

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, GetADirectoryPathWithCachedSourceContextId)
{
    SourceContextId sourceContextId{5};
    cache.sourceContextPath(sourceContextId);

    auto sourceContextPath = cache.sourceContextPath(sourceContextId);

    ASSERT_THAT(sourceContextPath, Eq(Utils::SmallStringView{"/path/to"}));
}

TEST_F(SourcePathCache, DirectoryPathCallsFetchDirectoryPath)
{
    EXPECT_CALL(storageMock, fetchSourceContextPath(Eq(SourceContextId{5})));

    cache.sourceContextPath(SourceContextId{5});
}

TEST_F(SourcePathCache, SecondDirectoryPathCallsNotFetchDirectoryPath)
{
    cache.sourceContextPath(SourceContextId{5});

    EXPECT_CALL(storageMock, fetchSourceContextPath(_)).Times(0);

    cache.sourceContextPath(SourceContextId{5});
}

TEST_F(SourcePathCache, ThrowForGettingASourceContextIdWithAnInvalidSourceId)
{
    SourceId sourceId;

    ASSERT_THROW(cache.sourceContextId(sourceId), QmlDesigner::NoSourcePathForInvalidSourceId);
}

TEST_F(SourcePathCache, FetchSourceContextIdBySourceId)
{
    auto sourceContextId = cache.sourceContextId(SourceId{42});

    ASSERT_THAT(sourceContextId, Eq(SourceContextId{5}));
}

TEST_F(SourcePathCache, FetchSourceContextIdBySourceIdCached)
{
    cache.sourceContextId(SourceId{42});

    auto sourceContextId = cache.sourceContextId(SourceId{42});

    ASSERT_THAT(sourceContextId, Eq(SourceContextId{5}));
}

TEST_F(SourcePathCache, FetchFilePathAfterFetchingSourceContextIdBySourceId)
{
    cache.sourceContextId(SourceId{42});

    auto sourcePath = cache.sourcePath(SourceId{42});

    ASSERT_THAT(sourcePath, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, FetchSourceContextIdAfterFetchingFilePathBySourceId)
{
    cache.sourcePath(SourceId{42});

    auto sourceContextId = cache.sourceContextId(SourceId{42});

    ASSERT_THAT(sourceContextId, Eq(SourceContextId{5}));
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

    ASSERT_THAT(id, Eq(SourceId{72}));
}

TEST_F(SourcePathCache, GetSourceContextIdInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto id = cacheFilled.sourceContextId(SourceId{42});

    ASSERT_THAT(id, Eq(SourceContextId{5}));
}

TEST_F(SourcePathCache, GetDirectoryPathInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourceContextPath(SourceContextId{5});

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, GetFilePathInFilledCache)
{
    Cache cacheFilled{storageMockFilled};

    auto path = cacheFilled.sourcePath(SourceId{42});

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

TEST_F(SourcePathCache, GetFileIdInAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto id = cacheNotFilled.sourceId("/path2/to/file.cpp");

    ASSERT_THAT(id, Eq(SourceId{72}));
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

    auto id = cacheNotFilled.sourceContextId(SourceId{42});

    ASSERT_THAT(id, Eq(SourceContextId{5}));
}

TEST_F(SourcePathCache, GetDirectoryPathAfterPopulateIfEmpty)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourceContextPath(SourceContextId{5});

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathCache, GetFilePathAfterPopulateIfEmptye)
{
    cacheNotFilled.populateIfEmpty();

    auto path = cacheNotFilled.sourcePath(SourceId{42});

    ASSERT_THAT(path, Eq("/path/to/file.cpp"));
}

} // namespace
