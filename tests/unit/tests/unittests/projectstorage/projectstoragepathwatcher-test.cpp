// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/filesystemmock.h"
#include "../mocks/mockqfilesystemwatcher.h"
#include "../mocks/mocktimer.h"
#include "../mocks/projectstoragepathwatchernotifiermock.h"

#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragepathwatcher.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>

#include <utils/smallstring.h>

namespace {
using SourcePathCache = QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>>;
using Watcher = QmlDesigner::ProjectStoragePathWatcher<NiceMock<MockQFileSytemWatcher>,
                                                       NiceMock<MockTimer>,
                                                       SourcePathCache>;
using QmlDesigner::FileStatus;
using QmlDesigner::IdPaths;
using QmlDesigner::ProjectChunkId;
using QmlDesigner::ProjectChunkIds;
using QmlDesigner::ProjectPartId;
using QmlDesigner::ProjectPartIds;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceContextIds;
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
    ProjectStoragePathWatcher()
    {
        ON_CALL(mockFileSystem, fileStatus(_)).WillByDefault([](auto sourceId) {
            return FileStatus{sourceId, 1, 1};
        });

        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath)))
            .WillByDefault(Return(SourceIds{sourceIds[0], sourceIds[1]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath2)))
            .WillByDefault(Return(SourceIds{sourceIds[2], sourceIds[3]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath3)))
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
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    SourcePathCache pathCache{storage};
    Watcher watcher{pathCache, mockFileSystem, &notifier};
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
    QString sourceContextPath = "/path";
    QString sourceContextPath2 = "/path2";
    QString sourceContextPath3 = "/path3";
    Utils::PathString sourceContextPathString = sourceContextPath;
    Utils::PathString sourceContextPathString2 = sourceContextPath2;
    SourceIds sourceIds = {pathCache.sourceId(path1),
                           pathCache.sourceId(path2),
                           pathCache.sourceId(path3),
                           pathCache.sourceId(path4),
                           pathCache.sourceId(path5)};
    SourceContextIds sourceContextIds = {pathCache.sourceContextId(sourceIds[0]),
                                         pathCache.sourceContextId(sourceIds[2]),
                                         pathCache.sourceContextId(sourceIds[4])};
    ProjectChunkIds ids{projectChunkId1, projectChunkId2, projectChunkId3};
    WatcherEntry watcherEntry1{projectChunkId1, sourceContextIds[0], sourceIds[0]};
    WatcherEntry watcherEntry2{projectChunkId2, sourceContextIds[0], sourceIds[0]};
    WatcherEntry watcherEntry3{projectChunkId1, sourceContextIds[0], sourceIds[1]};
    WatcherEntry watcherEntry4{projectChunkId2, sourceContextIds[0], sourceIds[1]};
    WatcherEntry watcherEntry5{projectChunkId3, sourceContextIds[0], sourceIds[1]};
    WatcherEntry watcherEntry6{projectChunkId1, sourceContextIds[1], sourceIds[2]};
    WatcherEntry watcherEntry7{projectChunkId2, sourceContextIds[1], sourceIds[3]};
    WatcherEntry watcherEntry8{projectChunkId3, sourceContextIds[1], sourceIds[3]};
    WatcherEntry watcherEntry9{projectChunkId4, sourceContextIds[0], sourceIds[0]};
    WatcherEntry watcherEntry10{projectChunkId4, sourceContextIds[0], sourceIds[1]};
    WatcherEntry watcherEntry11{projectChunkId4, sourceContextIds[1], sourceIds[2]};
    WatcherEntry watcherEntry12{projectChunkId4, sourceContextIds[1], sourceIds[3]};
    WatcherEntry watcherEntry13{projectChunkId4, sourceContextIds[2], sourceIds[4]};
};

TEST_F(ProjectStoragePathWatcher, add_id_paths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(
                    UnorderedElementsAre(QString(sourceContextPath), QString(sourceContextPath2))));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_calls_add_path_in_file_watcher)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(UnorderedElementsAre(QString(sourceContextPath2))));

    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, update_id_paths_and_remove_unused_paths_calls_remove_path_in_file_watcher)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(UnorderedElementsAre(QString(sourceContextPath2))));

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
                addPaths(ElementsAre(sourceContextPath, sourceContextPath2, sourceContextPath3)));

    watcher.updateIdPaths(
        {{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, add_entries_with_different_id_and_same_paths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(ElementsAre(sourceContextPath)));

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

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(sourceContextPath2)));

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
                removePaths(ElementsAre(sourceContextPath, sourceContextPath2)));

    watcher.removeIds({projectChunkId1.id, projectChunkId2.id, projectChunkId3.id});
}

TEST_F(ProjectStoragePathWatcher, remove_one_path_for_two_id)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1]}},
                           {projectChunkId3, {sourceIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(sourceContextPath)));

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
                pathsWithIdsChanged(ElementsAre(
                    IdPaths{projectChunkId1, {SourceId::create(1), SourceId::create(2)}},
                    IdPaths{projectChunkId2,
                            {SourceId::create(1), SourceId::create(2), SourceId::create(4)}})));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
    mockQFileSytemWatcher.directoryChanged(sourceContextPath2);
}

TEST_F(ProjectStoragePathWatcher, notify_for_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));

    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[3])))
        .WillByDefault(Return(FileStatus{sourceIds[3], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}

TEST_F(ProjectStoragePathWatcher, no_notify_for_unwatched_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[3]}}, {projectChunkId2, {sourceIds[3]}}});

    EXPECT_CALL(notifier, pathsChanged(IsEmpty()));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}

TEST_F(ProjectStoragePathWatcher, no_duplicate_path_changes)
{
    watcher.updateIdPaths({{projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
                           {projectChunkId2, {sourceIds[0], sourceIds[1], sourceIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(sourceIds[0])))
        .WillByDefault(Return(FileStatus{sourceIds[0], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(sourceIds[0])));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}

TEST_F(ProjectStoragePathWatcher, update_context_id_paths_adds_entry_in_new_directory)
{
    watcher.updateIdPaths({
        {projectChunkId1, {sourceIds[0], sourceIds[1], sourceIds[2]}},
        {projectChunkId4, {sourceIds[0], sourceIds[1], sourceIds[2], sourceIds[3]}},
    });

    watcher.updateContextIdPaths({{projectChunkId4, {sourceIds[4]}}}, {sourceContextIds[2]});

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
                                 {sourceContextIds[1]});

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

    watcher.updateContextIdPaths({{projectChunkId4, {sourceIds[3]}}}, {sourceContextIds[1]});

    ASSERT_THAT(watcher.watchedEntries(),
                UnorderedElementsAre(watcherEntry1,
                                     watcherEntry3,
                                     watcherEntry6,
                                     watcherEntry9,
                                     watcherEntry10,
                                     watcherEntry12));
}

} // namespace
