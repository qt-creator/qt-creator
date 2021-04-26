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

#include "mockfilepathstorage.h"
#include "mockmutex.h"
#include "sqlitedatabasemock.h"

#include <metainfo/storagecache.h>

#include <utils/smallstringio.h>

namespace {

using QmlDesigner::StorageCacheException;

using uint64 = unsigned long long;

using QmlDesigner::findInSorted;
using Utils::compare;
using Utils::reverseCompare;
using StorageIdFunction = std::function<int(Utils::SmallStringView)>;
using StorageStringFunction = std::function<Utils::PathString(int)>;

using CacheWithMockLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                       Utils::SmallStringView,
                                                       int,
                                                       NiceMock<MockMutex>,
                                                       decltype(&Utils::reverseCompare),
                                                       Utils::reverseCompare>;

using CacheWithoutLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                      Utils::SmallStringView,
                                                      int,
                                                      NiceMock<MockMutexNonLocking>,
                                                      decltype(&Utils::reverseCompare),
                                                      Utils::reverseCompare>;

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

        ON_CALL(this->mockStorage, fetchDirectoryId(Eq("foo"))).WillByDefault(Return(42));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq("bar"))).WillByDefault(Return(43));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq("poo"))).WillByDefault(Return(44));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq("taa"))).WillByDefault(Return(45));
        ON_CALL(this->mockStorage, fetchDirectoryPath(41)).WillByDefault(Return(Utils::PathString("bar")));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq(filePath1))).WillByDefault(Return(0));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq(filePath2))).WillByDefault(Return(1));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq(filePath3))).WillByDefault(Return(2));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq(filePath4))).WillByDefault(Return(3));
        ON_CALL(this->mockStorage, fetchDirectoryId(Eq(filePath5))).WillByDefault(Return(4));
    }

protected:
    NiceMock<SqliteDatabaseMock> databaseMock;
    NiceMock<MockFilePathStorage> mockStorage{databaseMock};
    StorageIdFunction mockStorageFetchDirectyId{
        [&](Utils::SmallStringView string) { return mockStorage.fetchDirectoryId(string); }};
    StorageStringFunction mockStorageFetchDirectyPath{
        [&](int id) { return mockStorage.fetchDirectoryPath(id); }};
    Cache cache{mockStorageFetchDirectyPath, mockStorageFetchDirectyId};
    typename Cache::MutexType &mockMutex = cache.mutex();
    Utils::PathString filePath1{"/file/pathOne"};
    Utils::PathString filePath2{"/file/pathTwo"};
    Utils::PathString filePath3{"/file/pathThree"};
    Utils::PathString filePath4{"/file/pathFour"};
    Utils::PathString filePath5{"/file/pathFife"};
    Utils::PathStringVector filePaths{filePath1,
                                      filePath2,
                                      filePath3,
                                      filePath4,
                                      filePath5};
    Utils::PathStringVector reverseFilePaths{filePath1,
                                             filePath2,
                                             filePath3,
                                             filePath4,
                                             filePath5};
};

using CacheTypes = ::testing::Types<CacheWithMockLocking, CacheWithoutLocking>;
TYPED_TEST_SUITE(StorageCache, CacheTypes);

TYPED_TEST(StorageCache, AddFilePath)
{
    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, 0);
}

TYPED_TEST(StorageCache, AddSecondFilePath)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath2);

    ASSERT_THAT(id, 1);
}

TYPED_TEST(StorageCache, AddDuplicateFilePath)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, 0);
}

TYPED_TEST(StorageCache, AddDuplicateFilePathBetweenOtherEntries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto id = this->cache.id(this->filePath3);

    ASSERT_THAT(id, 2);
}

TYPED_TEST(StorageCache, GetFilePathForIdWithOneEntry)
{
    this->cache.id(this->filePath1);

    auto filePath = this->cache.value(0);

    ASSERT_THAT(filePath, this->filePath1);
}

TYPED_TEST(StorageCache, GetFilePathForIdWithSomeEntries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePath = this->cache.value(2);

    ASSERT_THAT(filePath, this->filePath3);
}

TYPED_TEST(StorageCache, GetAllFilePaths)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePaths = this->cache.values({0, 1, 2, 3});

    ASSERT_THAT(filePaths,
                ElementsAre(this->filePath1, this->filePath2, this->filePath3, this->filePath4));
}

TYPED_TEST(StorageCache, AddFilePaths)
{
    auto ids = this->cache.ids({this->filePath1, this->filePath2, this->filePath3, this->filePath4});

    ASSERT_THAT(ids, ElementsAre(0, 1, 2, 3));
}

TYPED_TEST(StorageCache, AddFilePathsWithStorageFunction)
{
    auto ids = this->cache.ids({"foo", "taa", "poo", "bar"});

    ASSERT_THAT(ids, UnorderedElementsAre(42, 43, 44, 45));
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
    typename TypeParam::CacheEntries entries;

    this->cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(this->cache.isEmpty());
}

TYPED_TEST(StorageCache, IsNotEmptyAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 3},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 5}};

    this->cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(!this->cache.isEmpty());
}

TYPED_TEST(StorageCache, GetEntryAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 1},
                                             {this->filePath3.clone(), 7},
                                             {this->filePath4.clone(), 3}};
    this->cache.uncheckedPopulate(std::move(entries));

    auto value = this->cache.value(7);

    ASSERT_THAT(value, this->filePath3);
}

TYPED_TEST(StorageCache, EntriesHaveUniqueIds)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 1},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 2}};

    ASSERT_THROW(this->cache.populate(std::move(entries)), StorageCacheException);
}

TYPED_TEST(StorageCache, MultipleEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath1.clone(), 1},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 3}};

    ASSERT_THROW(this->cache.populate(std::move(entries)), StorageCacheException);
}

TYPED_TEST(StorageCache, DontFindInSorted)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFoo", compare);

    ASSERT_FALSE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedOne)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathOne", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedTwo)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathTwo", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedTree)
{
    auto found = findInSorted(this->filePaths.cbegin(),
                              this->filePaths.cend(),
                              "/file/pathThree",
                              compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedFour)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFour", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedFife)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFife", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, DontFindInSortedReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFoo",
                              reverseCompare);

    ASSERT_FALSE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedOneReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathOne",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedTwoReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathTwo",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedTreeReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathThree",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedFourReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFour",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StorageCache, FindInSortedFifeReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFife",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
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
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));
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
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo"))).Times(0);
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
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(41)));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.value(41);
}

TYPED_TEST(StorageCache, ValueWithStorageFunctionIsReadLockedForKnownId)
{
    InSequence s;
    this->cache.value(41);

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(41))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.value(41);
}

TYPED_TEST(StorageCache, IdWithStorageFunctionWhichHasNoEntryIsCallingStorageFunction)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IdWithStorageFunctionWhichHasEntryIsNotCallingStorageFunction)
{
    this->cache.id("foo");

    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo"))).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, IndexOfIdWithStorageFunctionWhichHasEntry)
{
    this->cache.id("foo");

    auto index = this->cache.id("foo");

    ASSERT_THAT(index, 42);
}

TYPED_TEST(StorageCache, IndexOfIdWithStorageFunctionWhichHasNoEntry)
{
    auto index = this->cache.id("foo");

    ASSERT_THAT(index, 42);
}

TYPED_TEST(StorageCache, GetEntryByIndexAfterInsertingByCustomIndex)
{
    auto index = this->cache.id("foo");

    auto value = this->cache.value(index);

    ASSERT_THAT(value, Eq("foo"));
}

TYPED_TEST(StorageCache, CallFetchDirectoryPathForLowerIndex)
{
    auto index = this->cache.id("foo");

    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(index - 1)));

    this->cache.value(index - 1);
}

TYPED_TEST(StorageCache, CallFetchDirectoryPathForUnknownIndex)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(0)));

    this->cache.value(0);
}

TYPED_TEST(StorageCache, FetchDirectoryPathForUnknownIndex)
{
    auto value = this->cache.value(41);

    ASSERT_THAT(value, Eq("bar"));
}

TYPED_TEST(StorageCache, AddCalls)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("bar")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("poo")));

    this->cache.add({"foo", "bar", "poo"});
}

TYPED_TEST(StorageCache, AddCallsOnlyForNewValues)
{
    this->cache.add({"foo", "poo"});

    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("taa")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("bar")));

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
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("bar")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.ids({"foo", "bar"});
}
} // namespace
