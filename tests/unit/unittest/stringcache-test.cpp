/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "mocksqlitedatabase.h"

#include <stringcache.h>

#include <utils/smallstringio.h>

namespace {

using ClangBackEnd::StringCacheException;

using uint64 = unsigned long long;

using Utils::compare;
using Utils::reverseCompare;
using ClangBackEnd::findInSorted;
using StorageIdFunction = std::function<int(Utils::SmallStringView)>;
using StorageStringFunction = std::function<Utils::PathString(int)>;

using CacheWithMockLocking = ClangBackEnd::StringCache<Utils::PathString,
                                                       Utils::SmallStringView,
                                                       int,
                                                       NiceMock<MockMutex>,
                                                       decltype(&Utils::reverseCompare),
                                                       Utils::reverseCompare>;

using CacheWithoutLocking = ClangBackEnd::StringCache<Utils::PathString,
                                                      Utils::SmallStringView,
                                                      int,
                                                      NiceMock<MockMutexNonLocking>,
                                                      decltype(&Utils::reverseCompare),
                                                      Utils::reverseCompare>;

template<typename Cache>
class StringCache : public testing::Test
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
    }

protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
    NiceMock<MockFilePathStorage> mockStorage{mockDatabase};
    StorageIdFunction mockStorageFetchDirectyId = [&](Utils::SmallStringView string) {
        return mockStorage.fetchDirectoryId(string);
    };
    StorageStringFunction mockStorageFetchDirectyPath = [&](int id) {
        return mockStorage.fetchDirectoryPath(id);
    };
    Cache cache;
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
TYPED_TEST_SUITE(StringCache, CacheTypes);

TYPED_TEST(StringCache, AddFilePath)
{
    auto id = this->cache.stringId(this->filePath1);

    ASSERT_THAT(id, 0);
}

TYPED_TEST(StringCache, AddSecondFilePath)
{
    this->cache.stringId(this->filePath1);

    auto id = this->cache.stringId(this->filePath2);

    ASSERT_THAT(id, 1);
}

TYPED_TEST(StringCache, AddDuplicateFilePath)
{
    this->cache.stringId(this->filePath1);

    auto id = this->cache.stringId(this->filePath1);

    ASSERT_THAT(id, 0);
}

TYPED_TEST(StringCache, AddDuplicateFilePathBetweenOtherEntries)
{
    this->cache.stringId(this->filePath1);
    this->cache.stringId(this->filePath2);
    this->cache.stringId(this->filePath3);
    this->cache.stringId(this->filePath4);

    auto id = this->cache.stringId(this->filePath3);

    ASSERT_THAT(id, 2);
}

TYPED_TEST(StringCache, ThrowForGettingPathForNoEntry)
{
    EXPECT_ANY_THROW(this->cache.string(0));
}

TYPED_TEST(StringCache, GetFilePathForIdWithOneEntry)
{
    this->cache.stringId(this->filePath1);

    auto filePath = this->cache.string(0);

    ASSERT_THAT(filePath, this->filePath1);
}

TYPED_TEST(StringCache, GetFilePathForIdWithSomeEntries)
{
    this->cache.stringId(this->filePath1);
    this->cache.stringId(this->filePath2);
    this->cache.stringId(this->filePath3);
    this->cache.stringId(this->filePath4);

    auto filePath = this->cache.string(2);

    ASSERT_THAT(filePath, this->filePath3);
}

TYPED_TEST(StringCache, GetAllFilePaths)
{
    this->cache.stringId(this->filePath1);
    this->cache.stringId(this->filePath2);
    this->cache.stringId(this->filePath3);
    this->cache.stringId(this->filePath4);

    auto filePaths = this->cache.strings({0, 1, 2, 3});

    ASSERT_THAT(filePaths,
                ElementsAre(this->filePath1, this->filePath2, this->filePath3, this->filePath4));
}

TYPED_TEST(StringCache, AddFilePaths)
{
    auto ids = this->cache.stringIds(
        {this->filePath1, this->filePath2, this->filePath3, this->filePath4});

    ASSERT_THAT(ids, ElementsAre(0, 1, 2, 3));
}

TYPED_TEST(StringCache, AddFilePathsWithStorageFunction)
{
    auto ids = this->cache.stringIds({"foo", "taa", "poo", "bar"}, this->mockStorageFetchDirectyId);

    ASSERT_THAT(ids, UnorderedElementsAre(42, 43, 44, 45));
}

TYPED_TEST(StringCache, IsEmpty)
{
    auto isEmpty = this->cache.isEmpty();

    ASSERT_TRUE(isEmpty);
}

TYPED_TEST(StringCache, IsNotEmpty)
{
    this->cache.stringId(this->filePath1);

    auto isEmpty = this->cache.isEmpty();

    ASSERT_FALSE(isEmpty);
}

TYPED_TEST(StringCache, PopulateWithEmptyVector)
{
    typename TypeParam::CacheEntries entries;

    this->cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(this->cache.isEmpty());
}

TYPED_TEST(StringCache, IsNotEmptyAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 3},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 5}};

    this->cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(!this->cache.isEmpty());
}

TYPED_TEST(StringCache, GetEntryAfterPopulateWithSomeEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 1},
                                             {this->filePath3.clone(), 7},
                                             {this->filePath4.clone(), 3}};
    this->cache.uncheckedPopulate(std::move(entries));

    auto string = this->cache.string(7);

    ASSERT_THAT(string, this->filePath3);
}

TYPED_TEST(StringCache, EntriesHaveUniqueIds)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath2.clone(), 1},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 2}};

    ASSERT_THROW(this->cache.populate(std::move(entries)), StringCacheException);
}

TYPED_TEST(StringCache, MultipleEntries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1.clone(), 0},
                                             {this->filePath1.clone(), 1},
                                             {this->filePath3.clone(), 2},
                                             {this->filePath4.clone(), 3}};

    ASSERT_THROW(this->cache.populate(std::move(entries)), StringCacheException);
}

TYPED_TEST(StringCache, DontFindInSorted)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFoo", compare);

    ASSERT_FALSE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedOne)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathOne", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedTwo)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathTwo", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedTree)
{
    auto found = findInSorted(this->filePaths.cbegin(),
                              this->filePaths.cend(),
                              "/file/pathThree",
                              compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedFour)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFour", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedFife)
{
    auto found = findInSorted(this->filePaths.cbegin(), this->filePaths.cend(), "/file/pathFife", compare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, DontFindInSortedReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFoo",
                              reverseCompare);

    ASSERT_FALSE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedOneReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathOne",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedTwoReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathTwo",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedTreeReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathThree",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedFourReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFour",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, FindInSortedFifeReverse)
{
    auto found = findInSorted(this->reverseFilePaths.cbegin(),
                              this->reverseFilePaths.cend(),
                              "/file/pathFife",
                              reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TYPED_TEST(StringCache, StringIdIsReadAndWriteLockedForUnknownEntry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.stringId("foo");
}

TYPED_TEST(StringCache, StringIdWithStorageFunctionIsReadAndWriteLockedForUnknownEntry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.stringId("foo", this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, StringIdWithStorageFunctionIsReadLockedForKnownEntry)
{
    InSequence s;
    this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo"))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.stringId("foo", this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, StringIdIsReadLockedForKnownEntry)
{
    this->cache.stringId("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.stringId("foo");
}

TYPED_TEST(StringCache, StringIdsIsLocked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.stringIds({"foo"});
}

TYPED_TEST(StringCache, StringIdsWithStorageIsLocked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.stringIds({"foo"}, this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, StringIsLocked)
{
    auto id = this->cache.stringId("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.string(id);
}

TYPED_TEST(StringCache, StringsIsLocked)
{
    auto ids = this->cache.stringIds({"foo", "bar"});

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.strings(ids);
}

TYPED_TEST(StringCache, StringWithStorageFunctionIsReadAndWriteLockedForUnknownId)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(41)));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.string(41, this->mockStorageFetchDirectyPath);
}

TYPED_TEST(StringCache, StringWithStorageFunctionIsReadLockedForKnownId)
{
    InSequence s;
    this->cache.string(41, this->mockStorageFetchDirectyPath);

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(41))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.string(41, this->mockStorageFetchDirectyPath);
}

TYPED_TEST(StringCache, StringIdWithStorageFunctionWhichHasNoEntryIsCallingStorageFunction)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));

    this->cache.stringId("foo", this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, StringIdWithStorageFunctionWhichHasEntryIsNotCallingStorageFunction)
{
    this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo"))).Times(0);

    this->cache.stringId("foo", this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, IndexOfStringIdWithStorageFunctionWhichHasEntry)
{
    this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    auto index = this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    ASSERT_THAT(index, 42);
}

TYPED_TEST(StringCache, IndexOfStringIdWithStorageFunctionWhichHasNoEntry)
{
    auto index = this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    ASSERT_THAT(index, 42);
}

TYPED_TEST(StringCache, GetEntryByIndexAfterInsertingByCustomIndex)
{
    auto index = this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    auto string = this->cache.string(index, this->mockStorageFetchDirectyPath);

    ASSERT_THAT(string, Eq("foo"));
}

TYPED_TEST(StringCache, CallFetchDirectoryPathForLowerIndex)
{
    auto index = this->cache.stringId("foo", this->mockStorageFetchDirectyId);

    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(index - 1)));

    this->cache.string(index - 1, this->mockStorageFetchDirectyPath);
}

TYPED_TEST(StringCache, CallFetchDirectoryPathForUnknownIndex)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(0)));

    this->cache.string(0, this->mockStorageFetchDirectyPath);
}

TYPED_TEST(StringCache, FetchDirectoryPathForUnknownIndex)
{
    auto string = this->cache.string(41, this->mockStorageFetchDirectyPath);

    ASSERT_THAT(string, Eq("bar"));
}

TYPED_TEST(StringCache, AddStringCalls)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("foo")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("bar")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("poo")));

    this->cache.addStrings({"foo", "bar", "poo"}, this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, AddStringCallsOnlyForNewStrings)
{
    this->cache.addStrings({"foo", "poo"}, this->mockStorageFetchDirectyId);

    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("taa")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryId(Eq("bar")));

    this->cache.addStrings({"foo", "bar", "poo", "taa"}, this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, GetStringIdAfterAddingStrings)
{
    this->cache.addStrings({"foo", "bar", "poo", "taa"}, this->mockStorageFetchDirectyId);

    ASSERT_THAT(this->cache.string(this->cache.stringId("taa")), Eq("taa"));
}

TYPED_TEST(StringCache, GetStringAfterAddingStrings)
{
    this->cache.addStrings({"foo", "bar", "poo", "taa"}, this->mockStorageFetchDirectyId);

    ASSERT_THAT(this->cache.string(this->cache.stringId("taa")), Eq("taa"));
}

TYPED_TEST(StringCache, GetStringIdAfterAddingStringsMultipleTimes)
{
    this->cache.addStrings({"foo", "taa"}, this->mockStorageFetchDirectyId);

    this->cache.addStrings({"foo", "bar", "poo", "taa"}, this->mockStorageFetchDirectyId);

    ASSERT_THAT(this->cache.string(this->cache.stringId("taa")), Eq("taa"));
}

TYPED_TEST(StringCache, GetStringIdAfterAddingTheSameStringsMultipleTimes)
{
    this->cache.addStrings({"foo", "taa", "poo", "taa", "bar", "taa"},
                           this->mockStorageFetchDirectyId);

    ASSERT_THAT(this->cache.string(this->cache.stringId("taa")), Eq("taa"));
}

TYPED_TEST(StringCache, AddingEmptyStrings)
{
    this->cache.addStrings({}, this->mockStorageFetchDirectyId);
}

TYPED_TEST(StringCache, FetchStringIdsFromStorageCalls)
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

    this->cache.stringIds({"foo", "bar"}, this->mockStorageFetchDirectyId);
}
} // namespace
