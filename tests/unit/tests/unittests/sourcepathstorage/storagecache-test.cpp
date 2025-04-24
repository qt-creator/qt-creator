// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/mockmutex.h"
#include "../mocks/projectstoragemock.h"
#include "../mocks/sqlitedatabasemock.h"

#include <sourcepathids.h>
#include <sourcepathstorage/storagecache.h>

#include <utils/smallstringio.h>

namespace {

using QmlDesigner::DirectoryPathId;
using QmlDesigner::StorageCacheException;
using Utils::compare;

class StorageAdapter
{
public:
    auto fetchId(Utils::SmallStringView view) const { return storage.fetchDirectoryPathId(view); }

    auto fetchValue(DirectoryPathId id) const { return storage.fetchDirectoryPath(id); }

    auto fetchAll() const { return storage.fetchAllDirectoryPaths(); }

    ProjectStorageMock &storage;
};

struct Less
{
    bool operator()(Utils::SmallStringView first, Utils::SmallStringView second) const
    {
        return Utils::compare(first, second) < 0;
    }
};

using CacheWithMockLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                       Utils::SmallStringView,
                                                       DirectoryPathId,
                                                       StorageAdapter,
                                                       NiceMock<MockMutex>,
                                                       Less,
                                                       QmlDesigner::Cache::DirectoryPath>;

using CacheWithoutLocking = QmlDesigner::StorageCache<Utils::PathString,
                                                      Utils::SmallStringView,
                                                      DirectoryPathId,
                                                      StorageAdapter,
                                                      NiceMock<MockMutexNonLocking>,
                                                      Less,
                                                      QmlDesigner::Cache::DirectoryPath>;

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
            return std::lexicographical_compare(f.rbegin(), f.rend(), l.rbegin(), l.rend());
        });

        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo"))).WillByDefault(Return(id42));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq("bar"))).WillByDefault(Return(id43));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq("poo"))).WillByDefault(Return(id44));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq("taa"))).WillByDefault(Return(id45));
        ON_CALL(this->mockStorage, fetchDirectoryPath(this->id41))
            .WillByDefault(Return(Utils::PathString("bar")));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq(filePath1))).WillByDefault(Return(id1));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq(filePath2))).WillByDefault(Return(id2));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq(filePath3))).WillByDefault(Return(id3));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq(filePath4))).WillByDefault(Return(id4));
        ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq(filePath5))).WillByDefault(Return(id5));
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
    DirectoryPathId id1{DirectoryPathId::create(1)};
    DirectoryPathId id2{DirectoryPathId::create(2)};
    DirectoryPathId id3{DirectoryPathId::create(3)};
    DirectoryPathId id4{DirectoryPathId::create(4)};
    DirectoryPathId id5{DirectoryPathId::create(5)};
    DirectoryPathId id41{DirectoryPathId::create(41)};
    DirectoryPathId id42{DirectoryPathId::create(42)};
    DirectoryPathId id43{DirectoryPathId::create(43)};
    DirectoryPathId id44{DirectoryPathId::create(44)};
    DirectoryPathId id45{DirectoryPathId::create(45)};
};

using CacheTypes = ::testing::Types<CacheWithMockLocking, CacheWithoutLocking>;
TYPED_TEST_SUITE(StorageCache, CacheTypes);

TYPED_TEST(StorageCache, add_file_path)
{
    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, this->id1);
}

TYPED_TEST(StorageCache, add_second_file_path)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath2);

    ASSERT_THAT(id, this->id2);
}

TYPED_TEST(StorageCache, add_duplicate_file_path)
{
    this->cache.id(this->filePath1);

    auto id = this->cache.id(this->filePath1);

    ASSERT_THAT(id, this->id1);
}

TYPED_TEST(StorageCache, add_duplicate_file_path_between_other_entries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto id = this->cache.id(this->filePath3);

    ASSERT_THAT(id, this->id3);
}

TYPED_TEST(StorageCache, get_file_path_for_id_with_one_entry)
{
    this->cache.id(this->filePath1);

    auto filePath = this->cache.value(this->id1);

    ASSERT_THAT(filePath, this->filePath1);
}

TYPED_TEST(StorageCache, get_file_path_for_id_with_some_entries)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePath = this->cache.value(this->id3);

    ASSERT_THAT(filePath, this->filePath3);
}

TYPED_TEST(StorageCache, get_all_file_paths)
{
    this->cache.id(this->filePath1);
    this->cache.id(this->filePath2);
    this->cache.id(this->filePath3);
    this->cache.id(this->filePath4);

    auto filePaths = this->cache.values({this->id1, this->id2, this->id3, this->id4});

    ASSERT_THAT(filePaths,
                ElementsAre(this->filePath1, this->filePath2, this->filePath3, this->filePath4));
}

TYPED_TEST(StorageCache, add_file_paths)
{
    auto ids = this->cache.ids({this->filePath1, this->filePath2, this->filePath3, this->filePath4});

    ASSERT_THAT(ids, ElementsAre(this->id1, this->id2, this->id3, this->id4));
}

TYPED_TEST(StorageCache, add_file_paths_with_storage_function)
{
    auto ids = this->cache.ids({"foo", "taa", "poo", "bar"});

    ASSERT_THAT(ids, UnorderedElementsAre(this->id42, this->id43, this->id44, this->id45));
}

TYPED_TEST(StorageCache, is_empty)
{
    auto isEmpty = this->cache.isEmpty();

    ASSERT_TRUE(isEmpty);
}

TYPED_TEST(StorageCache, is_not_empty)
{
    this->cache.id(this->filePath1);

    auto isEmpty = this->cache.isEmpty();

    ASSERT_FALSE(isEmpty);
}

TYPED_TEST(StorageCache, populate_with_empty_vector)
{
    this->cache.uncheckedPopulate();

    ASSERT_TRUE(this->cache.isEmpty());
}

TYPED_TEST(StorageCache, is_not_empty_after_populate_with_some_entries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1, this->id1},
                                             {this->filePath2, this->id4},
                                             {this->filePath3, this->id3},
                                             {this->filePath4, DirectoryPathId::create(5)}};
    ON_CALL(this->mockStorage, fetchAllDirectoryPaths()).WillByDefault(Return(entries));

    this->cache.uncheckedPopulate();

    ASSERT_TRUE(!this->cache.isEmpty());
}

TYPED_TEST(StorageCache, get_entry_after_populate_with_some_entries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1, this->id1},
                                             {this->filePath2, this->id2},
                                             {this->filePath3, DirectoryPathId::create(7)},
                                             {this->filePath4, this->id4}};
    ON_CALL(this->mockStorage, fetchAllDirectoryPaths()).WillByDefault(Return(entries));
    this->cache.uncheckedPopulate();

    auto value = this->cache.value(DirectoryPathId::create(7));

    ASSERT_THAT(value, this->filePath3);
}

TYPED_TEST(StorageCache, entries_have_unique_ids)
{
    typename TypeParam::CacheEntries entries{{this->filePath1, this->id1},
                                             {this->filePath2, this->id2},
                                             {this->filePath3, this->id3},
                                             {this->filePath4, this->id3}};
    ON_CALL(this->mockStorage, fetchAllDirectoryPaths()).WillByDefault(Return(entries));

    ASSERT_THROW(this->cache.populate(), StorageCacheException);
}

TYPED_TEST(StorageCache, multiple_entries)
{
    typename TypeParam::CacheEntries entries{{this->filePath1, this->id1},
                                             {this->filePath1, this->id2},
                                             {this->filePath3, this->id3},
                                             {this->filePath4, this->id4}};
    ON_CALL(this->mockStorage, fetchAllDirectoryPaths()).WillByDefault(Return(entries));

    ASSERT_THROW(this->cache.populate(), StorageCacheException);
}

TYPED_TEST(StorageCache, id_is_read_and_write_locked_for_unknown_entry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, id_with_storage_function_is_read_and_write_locked_for_unknown_entry)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, id_with_storage_function_is_read_locked_for_known_entry)
{
    InSequence s;
    this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo"))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, id_is_read_locked_for_known_entry)
{
    this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, ids_is_locked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.ids({"foo"});
}

TYPED_TEST(StorageCache, ids_with_storage_is_locked)
{
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.ids({"foo"});
}

TYPED_TEST(StorageCache, value_is_locked)
{
    auto id = this->cache.id("foo");

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.value(id);
}

TYPED_TEST(StorageCache, values_is_locked)
{
    auto ids = this->cache.ids({"foo", "bar"});

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());

    this->cache.values(ids);
}

TYPED_TEST(StorageCache, value_with_storage_function_is_read_and_write_locked_for_unknown_id)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(this->id41)));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.value(this->id41);
}

TYPED_TEST(StorageCache, value_with_storage_function_is_read_locked_for_known_id)
{
    InSequence s;
    this->cache.value(this->id41);

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock()).Times(0);
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(this->id41))).Times(0);
    EXPECT_CALL(this->mockMutex, unlock()).Times(0);

    this->cache.value(this->id41);
}

TYPED_TEST(StorageCache, id_with_storage_function_which_has_no_entry_is_calling_storage_function)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo")));

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, id_with_storage_function_which_has_entry_is_not_calling_storage_function)
{
    this->cache.id("foo");

    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo"))).Times(0);

    this->cache.id("foo");
}

TYPED_TEST(StorageCache, index_of_id_with_storage_function_which_has_entry)
{
    this->cache.id("foo");

    auto index = this->cache.id("foo");

    ASSERT_THAT(index, this->id42);
}

TYPED_TEST(StorageCache, index_of_id_with_storage_function_which_has_no_entry)
{
    auto index = this->cache.id("foo");

    ASSERT_THAT(index, this->id42);
}

TYPED_TEST(StorageCache, get_entry_by_index_after_inserting_by_custom_index)
{
    auto index = this->cache.id("foo");

    auto value = this->cache.value(index);

    ASSERT_THAT(value, Eq("foo"));
}

TYPED_TEST(StorageCache, call_fetch_directory_path_path_for_lower_index)
{
    auto index = this->cache.id("foo");
    DirectoryPathId lowerIndex{DirectoryPathId::create(index.internalId() - 1)};

    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(lowerIndex)));

    this->cache.value(lowerIndex);
}

TYPED_TEST(StorageCache, call_fetch_directory_path_path_for_unknown_index)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryPath(Eq(this->id1)));

    this->cache.value(this->id1);
}

TYPED_TEST(StorageCache, fetch_directory_path_path_for_unknown_index)
{
    auto value = this->cache.value(this->id41);

    ASSERT_THAT(value, Eq("bar"));
}

TYPED_TEST(StorageCache, add_calls)
{
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("bar")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("poo")));

    this->cache.add({"foo", "bar", "poo"});
}

TYPED_TEST(StorageCache, add_calls_only_for_new_values)
{
    this->cache.add({"foo", "poo"});

    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("taa")));
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("bar")));

    this->cache.add({"foo", "bar", "poo", "taa"});
}

TYPED_TEST(StorageCache, get_id_after_adding_values)
{
    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, get_value_after_adding_values)
{
    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, get_id_after_adding_values_multiple_times)
{
    this->cache.add({"foo", "taa"});

    this->cache.add({"foo", "bar", "poo", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, get_id_after_adding_the_same_values_multiple_times)
{
    this->cache.add({"foo", "taa", "poo", "taa", "bar", "taa"});

    ASSERT_THAT(this->cache.value(this->cache.id("taa")), Eq("taa"));
}

TYPED_TEST(StorageCache, adding_empty_values)
{
    this->cache.add({});
}

TYPED_TEST(StorageCache, fetch_ids_from_storage_calls)
{
    InSequence s;

    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("foo")));
    EXPECT_CALL(this->mockMutex, unlock());
    EXPECT_CALL(this->mockMutex, lock_shared());
    EXPECT_CALL(this->mockMutex, unlock_shared());
    EXPECT_CALL(this->mockMutex, lock());
    EXPECT_CALL(this->mockStorage, fetchDirectoryPathId(Eq("bar")));
    EXPECT_CALL(this->mockMutex, unlock());

    this->cache.ids({"foo", "bar"});
}

TYPED_TEST(StorageCache, getting_ids_that_start_with_po)
{
    ON_CALL(this->mockStorage, fetchDirectoryPathId(Eq("poh"))).WillByDefault(Return(this->id43));
    this->cache.add({"poo", "foo", "poh", "taa"});
    auto projection = [](Utils::SmallStringView text) -> Utils::SmallStringView {
        return text.substr(0, 2);
    };

    auto ids = this->cache.template ids<4>("po", projection);

    ASSERT_THAT(ids, UnorderedElementsAre(this->id43, this->id44));
}

TYPED_TEST(StorageCache, getting_no_ids_if_there_is_no_entry_starts_with_oh)
{
    this->cache.add({"poo", "taa", "foo", "bar"});
    auto projection = [](Utils::SmallStringView text) -> Utils::SmallStringView {
        return text.substr(0, 2);
    };

    auto ids = this->cache.template ids<4>("oh", projection);

    ASSERT_THAT(ids, IsEmpty());
}

TYPED_TEST(StorageCache, get_all_ids_if_the_string_is_empty)
{
    this->cache.add({"foo", "bar", "poo", "taa"});
    auto projection = [](Utils::SmallStringView text) -> Utils::SmallStringView {
        return text.substr(0, 0);
    };

    auto ids = this->cache.template ids<4>("", projection);

    ASSERT_THAT(ids, UnorderedElementsAre(this->id42, this->id43, this->id44, this->id45));
}
} // namespace
