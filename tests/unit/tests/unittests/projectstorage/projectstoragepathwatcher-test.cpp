// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <filesystemmock.h>
#include <mockqfilesystemwatcher.h>
#include <mocktimer.h>
#include <projectstorageerrornotifiermock.h>
#include <projectstoragepathwatchernotifiermock.h>

#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragepathwatcher.h>
#include <sourcepathstorage/sourcepathcache.h>
#include <sqlitedatabase.h>

#include <utils/smallstring.h>

namespace {
using SourcePathCache = QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage>;
using Watcher = QmlDesigner::ProjectStoragePathWatcher<NiceMock<MockQFileSytemWatcher>,
                                                       NiceMock<MockTimer>,
                                                       SourcePathCache>;
using QmlDesigner::FileStatus;
using QmlDesigner::IdPaths;
using QmlDesigner::ProjectChunkId;
using QmlDesigner::ProjectChunkIds;
using QmlDesigner::ProjectPartId;
using QmlDesigner::ProjectPartIds;
using QmlDesigner::DirectoryPathId;
using QmlDesigner::DirectoryPathIds;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::SourcePath;
using QmlDesigner::SourcePathView;
using QmlDesigner::SourceType;
using QmlDesigner::WatcherEntries;
using QmlDesigner::WatcherEntry;

class ProjectStoragePathWatcher : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
        ProjectStorageErrorNotifierMock errorNotifierMock;
        QmlDesigner::ProjectStorage storage{database, errorNotifierMock, database.isInitialized()};
        QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                         sourcePathDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    ~ProjectStoragePathWatcher() { storage.resetForTestsOnly(); }

    ProjectStoragePathWatcher()
    {
        ON_CALL(mockFileSystem, fileStatus(_)).WillByDefault([](auto sourceId) {
            return FileStatus{sourceId, 1, 1};
        });

        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath)))
            .WillByDefault(Return(SourceIds{sourceIds[0], sourceIds[1]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath2)))
            .WillByDefault(Return(SourceIds{sourceIds[2], sourceIds[3]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath3)))
            .WillByDefault(Return(SourceIds{sourceIds[4]}));
    }

    static WatcherEntries sorted(WatcherEntries &&entries)
    {
        std::stable_sort(entries.begin(), entries.end());

        return std::move(entries);
    }

protected:
    NiceMock<ProjectStoragePathWatcherNotifierMock> notifier;
    NiceMock<FileSystemMock> mockFileSystem;
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    QmlDesigner::ProjectStorage &storage = staticData->storage;
    SourcePathCache pathCache{staticData->sourcePathStorage};
    QmlDesigner::FileStatusCache fileStatusCache{mockFileSystem};
    Watcher watcher{pathCache, fileStatusCache, &notifier};
    NiceMock<MockQFileSytemWatcher> &mockQFileSytemWatcher = watcher.fileSystemWatcher();
    ProjectChunkId projectChunkId1{ProjectPartId::create(2), SourceType::Qml};
    ProjectChunkId projectChunkId2{ProjectPartId::create(2), SourceType::QmlUi};
    ProjectChunkId projectChunkId3{ProjectPartId::create(3), SourceType::QmlTypes};
    ProjectChunkId projectChunkId4{ProjectPartId::create(4), SourceType::Qml};
    SourcePathView path1{"/path/path1"};
    SourcePathView path2{"/path/path2"};
    SourcePathView path3{"/path2/path1"};
    SourcePathView path4{"/path2/path2"};
    SourcePathView path5{"/path3/path"};
    QString path1QString = QString(path1.toStringView());
    QString path2QString = QString(path2.toStringView());
    QString directoryPath = "/path";
    QString directoryPath2 = "/path2";
    QString directoryPath3 = "/path3";
    Utils::PathString directoryPathString = directoryPath;
    Utils::PathString directoryPathString2 = directoryPath2;
    SourceIds sourceIds = {pathCache.sourceId(path1),
                           pathCache.sourceId(path2),
                           pathCache.sourceId(path3),
                           pathCache.sourceId(path4),
                           pathCache.sourceId(path5)};
    DirectoryPathIds directoryPathIds = {sourceIds[0].directoryPathId(),
                                         sourceIds[2].directoryPathId(),
                                         sourceIds[4].directoryPathId()};
    ProjectChunkIds ids{projectChunkId1, projectChunkId2, projectChunkId3};
    WatcherEntry watcherEntry1{projectChunkId1, directoryPathIds[0], sourceIds[0]};
    WatcherEntry watcherEntry2{projectChunkId2, directoryPathIds[0], sourceIds[0]};
    WatcherEntry watcherEntry3{projectChunkId1, directoryPathIds[0], sourceIds[1]};
    WatcherEntry watcherEntry4{projectChunkId2, directoryPathIds[0], sourceIds[1]};
    WatcherEntry watcherEntry5{projectChunkId3, directoryPathIds[0], sourceIds[1]};
    WatcherEntry watcherEntry6{projectChunkId1, directoryPathIds[1], sourceIds[2]};
    WatcherEntry watcherEntry7{projectChunkId2, directoryPathIds[1], sourceIds[3]};
    WatcherEntry watcherEntry8{projectChunkId3, directoryPathIds[1], sourceIds[3]};
    WatcherEntry watcherEntry9{projectChunkId4, directoryPathIds[0], sourceIds[0]};
    WatcherEntry watcherEntry10{projectChunkId4, directoryPathIds[0], sourceIds[1]};
    WatcherEntry watcherEntry11{projectChunkId4, directoryPathIds[1], sourceIds[2]};
    WatcherEntry watcherEntry12{projectChunkId4, directoryPathIds[1], sourceIds[3]};
    WatcherEntry watcherEntry13{projectChunkId4, directoryPathIds[2], sourceIds[4]};
};

TEST_F(ProjectStoragePathWatcher, add_id_paths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(
                    UnorderedElementsAre(QString(directoryPath), QString(directoryPath2))));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_calls_add_path_in_file_watcher)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(UnorderedElementsAre(QString(directoryPath2))));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_and_remove_unused_paths_calls_remove_path_in_file_watcher)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(UnorderedElementsAre(QString(directoryPath2))));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_and_remove_unused_paths_do_not_calls_remove_path_in_file_watcher)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}},
                           {projectChunkId3, {sourceIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[1]}}, {projectChunkId2, {sourceIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_and_remove_unused_paths)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId3, {sourceIds[1]}}});

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0]}}, {projectChunkId2, {sourceIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ProjectStoragePathWatcher, extract_sorted_entries_from_convert_id_paths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds(
        {{projectChunkId2, {sourceIds[0], sourceIds[1]}},
         {projectChunkId1, {sourceIds[0], sourceIds[1]}}});

    ASSERT_THAT(entriesAndIds.first,
                ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, extract_sorted_ids_from_convert_id_paths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds(
        {{projectChunkId2, {}}, {projectChunkId1, {}}, {projectChunkId3, {}}});

    ASSERT_THAT(entriesAndIds.second, ElementsAre(ids[0], ids[1], ids[2]));
}

TEST_F(ProjectStoragePathWatcher, merge_entries)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0]}}, {projectChunkId2, {sourceIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, merge_more_entries)
{
    watcher.updateIdPaths({{projectChunkId2, {sourceIds[0], sourceIds[1]}}});

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(),
                ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, add_empty_entries)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths({});
}

TEST_F(ProjectStoragePathWatcher, add_entries_with_same_id_and_different_paths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(ElementsAre(directoryPath, directoryPath2, directoryPath3)));

    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, add_entries_with_different_id_and_same_paths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(ElementsAre(directoryPath)));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}}});
}

TEST_F(ProjectStoragePathWatcher, dont_add_new_entries_with_same_id_and_same_paths)
{
    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, dont_add_new_entries_with_different_id_and_same_paths)
{
    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths(
        {{projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, remove_entries_with_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId3, {sourceIds[1], sourceIds[3]}}});

    watcher.removeIds({ProjectPartId::create(2)});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry5, watcherEntry8));
}

TEST_F(ProjectStoragePathWatcher, remove_no_paths_for_empty_ids)
{
    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.removeIds({});
}

TEST_F(ProjectStoragePathWatcher, remove_no_paths_for_one_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.removeIds({projectChunkId3.id});
}

TEST_F(ProjectStoragePathWatcher, remove_path_for_one_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId3, {sourceIds[0], sourceIds[1], sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(directoryPath2)));

    watcher.removeIds({projectChunkId3.id});
}

TEST_F(ProjectStoragePathWatcher, remove_no_path_second_time)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    watcher.removeIds({projectChunkId2.id});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.removeIds({projectChunkId2.id});
}

TEST_F(ProjectStoragePathWatcher, remove_all_paths_for_three_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher,
                removePaths(ElementsAre(directoryPath, directoryPath2)));

    watcher.removeIds({projectChunkId1.id, projectChunkId2.id, projectChunkId3.id});
}

TEST_F(ProjectStoragePathWatcher, remove_one_path_for_two_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId3, {sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(directoryPath)));

    watcher.removeIds({projectChunkId1.id, projectChunkId2.id});
}

TEST_F(ProjectStoragePathWatcher, not_anymore_watched_entries_with_id)
{
    auto notContainsdId = [&](WatcherEntry entry) {
        return entry.id != ids[0] && entry.id != ids[1];
    };
    watcher.addEntries(
        sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    auto oldEntries = watcher.notAnymoreWatchedEntriesWithIds({watcherEntry1, watcherEntry4},
                                                              notContainsdId);

    ASSERT_THAT(oldEntries, ElementsAre(watcherEntry2, watcherEntry3));
}

TEST_F(ProjectStoragePathWatcher, remove_unused_entries)
{
    watcher.addEntries(
        sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    watcher.removeFromWatchedEntries({watcherEntry2, watcherEntry3});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ProjectStoragePathWatcher, two_notify_file_changes)
{
    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
         {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}},
         {projectChunkId3, {sourceIds[4]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[1])))
        .WillByDefault(Return(FileStatus{sourceIds[1], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 1, 2}));

    EXPECT_CALL(notifier,
                pathsWithIdsChanged(
                    ElementsAre(IdPaths{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                                IdPaths{projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}})));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
    mockQFileSytemWatcher.directoryChanged(directoryPath2);
}

TEST_F(ProjectStoragePathWatcher, notify_for_path_changes_if_modified_time_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ProjectStoragePathWatcher, notify_for_path_changes_if_size_get_bigger)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 2, 1}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 2, 1}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ProjectStoragePathWatcher, notify_for_path_changes_if_size_get_smaller)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 0, 1}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 0, 1}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ProjectStoragePathWatcher, no_notify_for_unwatched_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[3]}}, {projectChunkId2, {sourceIds[3]}}});

    EXPECT_CALL(notifier, pathsChanged(IsEmpty()));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ProjectStoragePathWatcher, no_duplicate_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ProjectStoragePathWatcher, trigger_manual_two_notify_file_changes)
{
    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
         {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3], sourceIds[4]}},
         {projectChunkId3, {sourceIds[4]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[1])))
        .WillByDefault(Return(FileStatus{sourceIds[1], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 1, 2}));

    EXPECT_CALL(notifier,
                pathsWithIdsChanged(
                    ElementsAre(IdPaths{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                                IdPaths{projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}})));

    watcher.checkForChangeInDirectory({sourceIds[0].directoryPathId(), sourceIds[2].directoryPathId()});
}

TEST_F(ProjectStoragePathWatcher, trigger_manual_notify_for_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    watcher.checkForChangeInDirectory({sourceIds[0].directoryPathId()});
}

TEST_F(ProjectStoragePathWatcher, trigger_manual_no_notify_for_unwatched_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[3]}}, {projectChunkId2, {sourceIds[3]}}});

    EXPECT_CALL(notifier, pathsChanged(IsEmpty()));

    watcher.checkForChangeInDirectory({sourceIds[0].directoryPathId()});
}

TEST_F(ProjectStoragePathWatcher, update_context_id_paths_adds_entry_in_new_directory)
{
    watcher.updateIdPaths({
        {projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
        {projectChunkId4, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3]}},
    });

    watcher.updateContextIdPaths({{projectChunkId4, {sourceIds[4]}}}, {directoryPathIds[2]});

    ASSERT_THAT(watcher.watchedEntries(),
                UnorderedElementsAre(watcherEntry1,
                                     watcherEntry3,
                                     watcherEntry6,
                                     watcherEntry9,
                                     watcherEntry10,
                                     watcherEntry11,
                                     watcherEntry12,
                                     watcherEntry13));
}

TEST_F(ProjectStoragePathWatcher, update_context_id_paths_adds_entry_to_directory)
{
    watcher.updateIdPaths({
        {projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
        {projectChunkId4, {sourceIds[1], sourceIds[3]}},
    });

    watcher.updateContextIdPaths({{projectChunkId4,
                                   {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3]}}},
                                 {directoryPathIds[1]});

    ASSERT_THAT(watcher.watchedEntries(),
                UnorderedElementsAre(watcherEntry1,
                                     watcherEntry3,
                                     watcherEntry6,
                                     watcherEntry9,
                                     watcherEntry10,
                                     watcherEntry11,
                                     watcherEntry12));
}

TEST_F(ProjectStoragePathWatcher, update_context_id_paths_removes_entry)
{
    watcher.updateIdPaths({
        {projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
        {projectChunkId4, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3]}},
    });

    watcher.updateContextIdPaths({{projectChunkId4, {sourceIds[3]}}}, {directoryPathIds[1]});

    ASSERT_THAT(watcher.watchedEntries(),
                UnorderedElementsAre(watcherEntry1,
                                     watcherEntry3,
                                     watcherEntry6,
                                     watcherEntry9,
                                     watcherEntry10,
                                     watcherEntry12));
}

} // namespace
