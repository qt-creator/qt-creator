// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sourcepathstorage/sourcepathstorage.h>

namespace {

using QmlDesigner::Cache::DirectoryPath;
using QmlDesigner::Cache::FileName;
using QmlDesigner::DirectoryPathId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::FileNameId;

MATCHER_P2(IsDirectoryPath,
           id,
           value,
           std::string(negation ? "isn't " : "is ") + PrintToString(DirectoryPath{value, id}))
{
    const DirectoryPath &directoryPath = arg;

    return directoryPath.id == id && directoryPath.value == value;
}

class SourcePathStorage : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::SourcePathStorage storage{database, database.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    SourcePathStorage() = default;

    ~SourcePathStorage() { storage.resetForTestsOnly(); }

    void addSomeDummyData()
    {
        storage.fetchDirectoryPathId("/path/dummy");
        storage.fetchDirectoryPathId("/path/dummy2");
        storage.fetchDirectoryPathId("/path/");

        storage.fetchFileNameId("foo");
        storage.fetchFileNameId("dummy");
        storage.fetchFileNameId("bar");
        storage.fetchFileNameId("bar");
    }

    template<typename Range>
    static auto toValues(Range &&range)
    {
        using Type = typename std::decay_t<Range>;

        return std::vector<typename Type::value_type>{range.begin(), range.end()};
    }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::SourcePathStorage &storage = staticData->storage;
    Sqlite::Database &database = staticData->database;
};

TEST_F(SourcePathStorage, fetch_directory_path_id_returns_always_the_same_id_for_the_same_path)

{
    auto directoryPathId = storage.fetchDirectoryPathId("/path/to");

    auto newDirectoryPathId = storage.fetchDirectoryPathId("/path/to");

    ASSERT_THAT(newDirectoryPathId, Eq(directoryPathId));
}

TEST_F(SourcePathStorage, fetch_directory_path_id_returns_not_the_same_id_for_different_path)
{
    auto directoryPathId = storage.fetchDirectoryPathId("/path/to");

    auto newDirectoryPathId = storage.fetchDirectoryPathId("/path/to2");

    ASSERT_THAT(newDirectoryPathId, Ne(directoryPathId));
}

TEST_F(SourcePathStorage, fetch_directory_path_path)
{
    auto directoryPathId = storage.fetchDirectoryPathId("/path/to");

    auto path = storage.fetchDirectoryPath(directoryPathId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathStorage, fetch_unknown_directory_path_path_throws)
{
    ASSERT_THROW(storage.fetchDirectoryPath(DirectoryPathId::create(323)),
                 QmlDesigner::DirectoryPathIdDoesNotExists);
}

TEST_F(SourcePathStorage, fetch_all_directory_paths_are_empty_if_no_directory_paths_exists)
{
    storage.clearSources();

    auto directoryPaths = storage.fetchAllDirectoryPaths();

    ASSERT_THAT(toValues(directoryPaths), IsEmpty());
}

TEST_F(SourcePathStorage, fetch_all_directory_paths)
{
    storage.clearSources();
    auto directoryPathId = storage.fetchDirectoryPathId("/path/to");
    auto directoryPathId2 = storage.fetchDirectoryPathId("/path/to2");

    auto directoryPaths = storage.fetchAllDirectoryPaths();

    ASSERT_THAT(toValues(directoryPaths),
                UnorderedElementsAre(IsDirectoryPath(directoryPathId, "/path/to"),
                                     IsDirectoryPath(directoryPathId2, "/path/to2")));
}

TEST_F(SourcePathStorage, fetch_source_id_first_time)
{
    addSomeDummyData();

    auto fileNameId = storage.fetchFileNameId("foo");

    ASSERT_TRUE(fileNameId.isValid());
}

TEST_F(SourcePathStorage, fetch_existing_source_id)
{
    addSomeDummyData();
    auto createdFileNameId = storage.fetchFileNameId("foo");

    auto fileNameId = storage.fetchFileNameId("foo");

    ASSERT_THAT(fileNameId, createdFileNameId);
}

TEST_F(SourcePathStorage, fetch_source_id_with_different_name_are_not_equal)
{
    addSomeDummyData();
    auto fileNameId2 = storage.fetchFileNameId("foo");

    auto fileNameId = storage.fetchFileNameId("foo2");

    ASSERT_THAT(fileNameId, Ne(fileNameId2));
}

TEST_F(SourcePathStorage, fetch_file_name_and_directory_path_id_for_non_existing_source_id)
{
    ASSERT_THROW(storage.fetchFileName(FileNameId::create(212)),
                 QmlDesigner::FileNameIdDoesNotExists);
}

TEST_F(SourcePathStorage, fetch_file_name_for_non_existing_entry)
{
    addSomeDummyData();
    auto fileNameId = storage.fetchFileNameId("foo");

    auto fileName = storage.fetchFileName(fileNameId);

    ASSERT_THAT(fileName, Eq("foo"));
}

TEST_F(SourcePathStorage, fetch_all_sources)
{
    storage.clearSources();

    auto sources = storage.fetchAllFileNames();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(SourcePathStorage, fetch_source_id_unguarded_first_time)
{
    addSomeDummyData();
    std::lock_guard lock{database};

    auto sourceId = storage.fetchFileNameIdUnguarded("foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(SourcePathStorage, fetch_existing_source_id_unguarded)
{
    addSomeDummyData();
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchFileNameIdUnguarded("foo");

    auto sourceId = storage.fetchFileNameIdUnguarded("foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(SourcePathStorage, fetch_source_id_unguarded_with_different_name_are_not_equal)
{
    addSomeDummyData();
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchFileNameIdUnguarded("foo");

    auto sourceId = storage.fetchFileNameIdUnguarded("foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}
} // namespace
