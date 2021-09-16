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

#include "mockmutex.h"
#include "projectstoragemock.h"
#include "sqlitedatabasemock.h"

#include <projectstorage/storagecache.h>
#include <projectstorageids.h>

#include <utils/smallstringio.h>

namespace {

using QmlDesigner::SourceContextId;
using QmlDesigner::StorageCacheException;
using Utils::compare;
using Utils::reverseCompare;

class StorageAdapter
{
public:
    auto fetchId(Utils::SmallStringView view) const { return storage.fetchSourceContextId(view); }

    auto fetchValue(SourceContextId id) const { return storage.fetchSourceContextPath(id); }

    auto fetchAll() const { return storage.fetchAllSourceContexts(); }

    ProjectStorageMock &storage;
};

auto less(Utils::SmallStringView first, Utils::SmallStringView second) -> bool
{
    return Utils::reverseCompare(first, second) < 0;
};

using CacheWithMockLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                       Utils::SmallStringView,
                                                       SourceContextId,
                                                       StorageAdapter,
                                                       NiceMock<MockMutex>,
                                                       less,
                                                       QmlDesigner::Cache::SourceContext>;

using CacheWithoutLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                      Utils::SmallStringView,
                                                      SourceContextId,
                                                      StorageAdapter,
                                                      NiceMock<MockMutexNonLocking>,
                                                      less,
                                                      QmlDesigner::Cache::SourceContext>;

template<typename Cache>
class StorageCache : public testing::Test
{
protected:
    void SetUp()
    {
        std::sort(filePaths.begin(), filePaths.end(), [](auto &f, auto &l) {
            return compare(f, l) < 0;
        });
        std::sort(reverseFilePaths.begin(), reverseFilePaths.end(), [](auto &f, auto &l) {
            return reverseCompare(f, l) < 0;
        });

        ON_CALL(this->mockStorage, fetchSourceContextId(Eq("foo"))).WillByDefault(Return(id42));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq("bar"))).WillByDefault(Return(id43));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq("poo"))).WillByDefault(Return(id44));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq("taa"))).WillByDefault(Return(id45));
        ON_CALL(this->mockStorage, fetchSourceContextPath(this->id41))
            .WillByDefault(Return(Utils::PathString("bar")));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq(filePath1))).WillByDefault(Return(id1));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq(filePath2))).WillByDefault(Return(id2));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq(filePath3))).WillByDefault(Return(id3));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq(filePath4))).WillByDefault(Return(id4));
        ON_CALL(this->mockStorage, fetchSourceContextId(Eq(filePath5))).WillByDefault(Return(id5));
    }

protected:
    NiceMock<ProjectStorageMock> mockStorage;
    StorageAdapter storageAdapter{mockStorage};
    Cache cache{storageAdapter};
    typename Cache::MutexType &mockMutex = cache.mutex();
    Utils::PathString filePath1{"/file/pathOne"};
    Utils::PathString filePath2{"/file/pathTwo"};
    Utils::PathString filePath3{"/file/pathThree"};
    Utils::PathString filePath4{"/file/pathFour"};
    Utils::PathString filePath5{"/file/pathFife"};
    Utils::PathStringVector filePaths{filePath1, filePath2, filePath3, filePath4, filePath5};
    Utils::PathStringVector reverseFilePaths{filePath1, filePath2, filePath3, filePath4, filePath5};
    SourceContextId id1{0};
    SourceContextId id2{1};
    SourceContextId id3{2};
    SourceContextId id4{3};
    SourceContextId id5{4};
    SourceContextId id41{41};
    SourceContextId id42{42};
    SourceContextId id43{43};
    SourceContextId id44{44};
    SourceContextId id45{45};
};

using CacheTypes = ::testing::Types<CacheWithMockLocking, CacheWithoutLocking>;
TYPED_TEST_SUITE(StorageCache, CacheTypes);

TYPED_TEST(StorageCache, AddFilePath)
{
    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, this->id1);
}

TYPED_TEST(StorageCache, AddSecondFilePath)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath2);

    ASSERT_THAT(id, this->id2);
}

TYPED_TEST(StorageCache, AddDuplicateFilePath)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, this->id1);
}

TYPED_TEST(StorageCache, AddDuplicateFilePathBetweenOtherEntries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto id = this->cache.id(this->filePath3);

    ASSERT_THAT(id, this->id3);
}

TYPED_TEST(StorageCache, GetFilePathForIdWithOneEntry)
{
    this->cache.id(this->filePath1);

    auto filePath = this->cache.value(this->id1);

    ASSERT_THAT(filePath, this->filePath1);
}

TYPED_TEST(StorageCache, GetFilePathForIdWithSomeEntries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePath = this->cache.value(this->id3);

    ASSERT_THAT(filePath, this->filePath3);
}

TYPED_TEST(StorageCache, GetAllFilePaths)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePaths = this->cache.values({this->id1, this->id2, this->id3, this->id4});

    ASSERT_THAT(filePaths,
                ElementsAre(this->filePath1, this->filePath2, this->filePath3, this->filePath4));
}

TYPED_TEST(StorageCache, AddFilePaths)
{
    auto ids = this->cache.ids({this->filePath1, this->filePath2, this->filePath3, this->filePath4});

    ASSERT_THAT(ids, ElementsAre(this->id1, this->id2, this->id3, this->id4));
}

TYPED_TEST(StorageCache, AddFilePathsWithStorageFunction)
{
    auto ids = this->cache.ids({"foo", "taa", "poo", "bar"});

    ASSERT_THAT(ids, UnorderedElementsAre(this->id42, this->id43, this->id44, this->id45));
}

TYPED_TEST(StorageCache, IsEmpty)
{
    auto isEmpty = this->cache.isEmpty();

    ASSERT_TRUE(isEmpty);
}

TYPED_TEST(StorageCache, IsNotEmpty)
{
    this->cache.id(this->filePath1);

    auto isEmpty = this->cache.isEmpty();

    ASSERT_FALSE(isEmpty);
}

TYPED_TEST(StorageCache, PopulateWithEmptyVector)
{
    this->cache.uncheckedPopulate();

    ASSERT_TRUE(this->cache.isEmpty());
}

TYPED_TEST(StorageCache, IsNotEmptyAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), this->id1},
                                             {this->filePath2.clone(), this->id4},
                                             {this->filePath3.clone(), this->id3},
                                             {this->filePath4.clone(), SourceContextId{5}}};
    ON_CALL(this->mockStorage, fetchAllSourceContexts()).WillByDefault(Return(entries));

    this->cache.uncheckedPopulate();

    ASSERT_TRUE(!this->cache.isEmpty());
}

TYPED_TEST(StorageCache, GetEntryAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), this->id1},
                                             {this->filePath2.clone(), this->id2},
                                             {this->filePath3.clone(), SourceContextId{7}},
                                             {this->filePath4.clone(), this->id4}};
    ON_CALL(this->mockStorage, fetchAllSourceContexts()).WillByDefault(Return(entries));
    this->cache.uncheckedPopulate();

    auto value = this->cache.value(SourceContextId{7});

    ASSERT_THAT(value, this->filePath3);
}

TYPED_TEST(StorageCache, EntriesHaveUniqueIds)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), this->id1},
                                             {this->filePath2.clone(), this->id2},
                                             {this->filePath3.clone(), this->id3},
                                             {this->filePath4.clone(), this->id3}};
    ON_CALL(this->mockStorage, fetchAllSourceContexts()).WillByDefault(Return(entries));

    ASSERT_THROW(this->cache.populate(), StorageCacheException);
}

TYPED_TEST(StorageCache, MultipleEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), this->id1},
                                             {this->filePath1.clone(), this->id2},
                                             {this->filePath3.clone(), this->id3},
                                             {this->filePath4.clone(), this->id4}};
    ON_CALL(this->mockStorage, fetchAllSourceContexts()).WillByDefault(Return(entries));

    ASSERT_THROW(this->cache.populate(), StorageCacheException);
}

TYPED_TEST(StorageCache, IdIsReadAndWriteLockedForUnknownEntry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdWithStorageFunctionIsReadAndWriteLockedForUnknownEntry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdWithStorageFunctionIsReadLockedForKnownEntry)
{
    InSequence s;
    this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo"))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdIsReadLockedForKnownEntry)
{
    this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdsIsLocked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.ids({"foo"});
}

TYPED_TEST(StorageCache, IdsWithStorageIsLocked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.ids({"foo"});
}

TYPED_TEST(StorageCache, ValueIsLocked)
{
    auto id = this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.value(id);
}

TYPED_TEST(StorageCache, ValuesIsLocked)
{
    auto ids = this->cache.ids({"foo", "bar"});

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.values(ids);
}

TYPED_TEST(StorageCache, ValueWithStorageFunctionIsReadAndWriteLockedForUnknownId)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchSourceContextPath(Eq(this->id41)));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.value(this->id41);
}

TYPED_TEST(StorageCache, ValueWithStorageFunctionIsReadLockedForKnownId)
{
    InSequence s;
    this->cache.value(this->id41);

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchSourceContextPath(Eq(this->id41))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.value(this->id41);
}

TYPED_TEST(StorageCache, IdWithStorageFunctionWhichHasNoEntryIsCallingStorageFunction)
{
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo")));

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdWithStorageFunctionWhichHasEntryIsNotCallingStorageFunction)
{
    this->cache.id("foo");

    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo"))).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IndexOfIdWithStorageFunctionWhichHasEntry)
{
    this->cache.id("foo");

    auto index = this->cache.id("foo");

    ASSERT_THAT(index, this->id42);
}

TYPED_TEST(StorageCache, IndexOfIdWithStorageFunctionWhichHasNoEntry)
{
    auto index = this->cache.id("foo");

    ASSERT_THAT(index, this->id42);
}

TYPED_TEST(StorageCache, GetEntryByIndexAfterInsertingByCustomIndex)
{
    auto index = this->cache.id("foo");

    auto value = this->cache.value(index);

    ASSERT_THAT(value, Eq("foo"));
}

TYPED_TEST(StorageCache, CallFetchSourceContextPathForLowerIndex)
{
    auto index = this->cache.id("foo");
    SourceContextId lowerIndex{&index - 1};

    EXPECT_CALL(this->mockStorage, fetchSourceContextPath(Eq(lowerIndex)));

    this->cache.value(lowerIndex);
}

TYPED_TEST(StorageCache, CallFetchSourceContextPathForUnknownIndex)
{
    EXPECT_CALL(this->mockStorage, fetchSourceContextPath(Eq(this->id1)));

    this->cache.value(this->id1);
}

TYPED_TEST(StorageCache, FetchSourceContextPathForUnknownIndex)
{
    auto value = this->cache.value(this->id41);

    ASSERT_THAT(value, Eq("bar"));
}

TYPED_TEST(StorageCache, AddCalls)
{
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo")));
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("bar")));
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("poo")));

    this->cache.add({"foo", "bar", "poo"});
}

TYPED_TEST(StorageCache, AddCallsOnlyForNewValues)
{
    this->cache.add({"foo", "poo"});

    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("taa")));
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("bar")));

    this->cache.add({"foo", "bar", "poo", "taa"});
}

TYPED_TEST(StorageCache, GetIdAfterAddingValues)
{
    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, GetValueAfterAddingValues)
{
    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, GetIdAfterAddingValuesMultipleTimes)
{
    this->cache.add({"foo", "taa"});

    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, GetIdAfterAddingTheSameValuesMultipleTimes)
{
    this->cache.add({"foo", "taa", "poo", "taa", "bar", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, AddingEmptyValues)
{
    this->cache.add({});
}

TYPED_TEST(StorageCache, FetchIdsFromStorageCalls)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchSourceContextId(Eq("bar")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.ids({"foo", "bar"});
}
} // namespace
