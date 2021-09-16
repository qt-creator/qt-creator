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

#include "filesystemmock.h"
#include "mockqfilesystemwatcher.h"
#include "mocktimer.h"
#include "projectstoragepathwatchernotifiermock.h"
#include "sourcepathcachemock.h"

#include <projectstorage/projectstoragepathwatcher.h>
#include <projectstorage/sourcepath.h>

#include <utils/smallstring.h>

namespace {

using Watcher = QmlDesigner::ProjectStoragePathWatcher<NiceMock<MockQFileSytemWatcher>,
                                                       NiceMock<MockTimer>,
                                                       NiceMock<SourcePathCacheMock>>;
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
        ON_CALL(sourcePathCacheMock, sourceId(Eq(path1))).WillByDefault(Return(pathIds[0]));
        ON_CALL(sourcePathCacheMock, sourceId(Eq(path2))).WillByDefault(Return(pathIds[1]));
        ON_CALL(sourcePathCacheMock, sourceId(Eq(path3))).WillByDefault(Return(pathIds[2]));
        ON_CALL(sourcePathCacheMock, sourceId(Eq(path4))).WillByDefault(Return(pathIds[3]));
        ON_CALL(sourcePathCacheMock, sourceId(Eq(path5))).WillByDefault(Return(pathIds[4]));
        ON_CALL(sourcePathCacheMock, sourcePath(Eq(pathIds[0]))).WillByDefault(Return(SourcePath{path1}));
        ON_CALL(sourcePathCacheMock, sourcePath(Eq(pathIds[1]))).WillByDefault(Return(SourcePath{path2}));
        ON_CALL(sourcePathCacheMock, sourcePath(Eq(pathIds[2]))).WillByDefault(Return(SourcePath{path3}));
        ON_CALL(sourcePathCacheMock, sourcePath(Eq(pathIds[3]))).WillByDefault(Return(SourcePath{path4}));
        ON_CALL(sourcePathCacheMock, sourcePath(Eq(pathIds[4]))).WillByDefault(Return(SourcePath{path5}));
        ON_CALL(sourcePathCacheMock, sourceContextId(TypedEq<SourceId>(pathIds[0])))
            .WillByDefault(Return(sourceContextIds[0]));
        ON_CALL(sourcePathCacheMock, sourceContextId(TypedEq<SourceId>(pathIds[1])))
            .WillByDefault(Return(sourceContextIds[0]));
        ON_CALL(sourcePathCacheMock, sourceContextId(TypedEq<SourceId>(pathIds[2])))
            .WillByDefault(Return(sourceContextIds[1]));
        ON_CALL(sourcePathCacheMock, sourceContextId(TypedEq<SourceId>(pathIds[3])))
            .WillByDefault(Return(sourceContextIds[1]));
        ON_CALL(sourcePathCacheMock, sourceContextId(TypedEq<SourceId>(pathIds[4])))
            .WillByDefault(Return(sourceContextIds[2]));
        ON_CALL(mockFileSystem, fileStatus(_)).WillByDefault([](auto sourceId) {
            return FileStatus{sourceId, 1, 1};
        });
        ON_CALL(sourcePathCacheMock,
                sourceContextId(TypedEq<Utils::SmallStringView>(sourceContextPathString)))
            .WillByDefault(Return(sourceContextIds[0]));
        ON_CALL(sourcePathCacheMock,
                sourceContextId(TypedEq<Utils::SmallStringView>(sourceContextPathString2)))
            .WillByDefault(Return(sourceContextIds[1]));
        ON_CALL(sourcePathCacheMock, sourceContextPath(Eq(sourceContextIds[0])))
            .WillByDefault(Return(sourceContextPath));
        ON_CALL(sourcePathCacheMock, sourceContextPath(Eq(sourceContextIds[1])))
            .WillByDefault(Return(sourceContextPath2));
        ON_CALL(sourcePathCacheMock, sourceContextPath(Eq(sourceContextIds[2])))
            .WillByDefault(Return(sourceContextPath3));
        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath)))
            .WillByDefault(Return(SourceIds{pathIds[0], pathIds[1]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath2)))
            .WillByDefault(Return(SourceIds{pathIds[2], pathIds[3]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(sourceContextPath3)))
            .WillByDefault(Return(SourceIds{pathIds[4]}));
    }
    static WatcherEntries sorted(WatcherEntries &&entries)
    {
        std::stable_sort(entries.begin(), entries.end());

        return std::move(entries);
    }

protected:
    NiceMock<SourcePathCacheMock> sourcePathCacheMock;
    NiceMock<ProjectStoragePathWatcherNotifierMock> notifier;
    NiceMock<FileSystemMock> mockFileSystem;
    Watcher watcher{sourcePathCacheMock, mockFileSystem, &notifier};
    NiceMock<MockQFileSytemWatcher> &mockQFileSytemWatcher = watcher.fileSystemWatcher();
    ProjectChunkId id1{ProjectPartId{2}, SourceType::Qml};
    ProjectChunkId id2{ProjectPartId{2}, SourceType::QmlUi};
    ProjectChunkId id3{ProjectPartId{4}, SourceType::QmlTypes};
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
    SourceIds pathIds = {SourceId{1}, SourceId{2}, SourceId{3}, SourceId{4}, SourceId{5}};
    SourceContextIds sourceContextIds = {SourceContextId{1}, SourceContextId{2}, SourceContextId{3}};
    ProjectChunkIds ids{id1, id2, id3};
    WatcherEntry watcherEntry1{id1, sourceContextIds[0], pathIds[0]};
    WatcherEntry watcherEntry2{id2, sourceContextIds[0], pathIds[0]};
    WatcherEntry watcherEntry3{id1, sourceContextIds[0], pathIds[1]};
    WatcherEntry watcherEntry4{id2, sourceContextIds[0], pathIds[1]};
    WatcherEntry watcherEntry5{id3, sourceContextIds[0], pathIds[1]};
    WatcherEntry watcherEntry6{id1, sourceContextIds[1], pathIds[2]};
    WatcherEntry watcherEntry7{id2, sourceContextIds[1], pathIds[3]};
    WatcherEntry watcherEntry8{id3, sourceContextIds[1], pathIds[3]};
};

TEST_F(ProjectStoragePathWatcher, AddIdPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(
                    UnorderedElementsAre(QString(sourceContextPath), QString(sourceContextPath2))));

    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, UpdateIdPathsCallsAddPathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(UnorderedElementsAre(QString(sourceContextPath2))));

    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, UpdateIdPathsAndRemoveUnusedPathsCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(UnorderedElementsAre(QString(sourceContextPath2))));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});
}

TEST_F(ProjectStoragePathWatcher, UpdateIdPathsAndRemoveUnusedPathsDoNotCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2]}},
                           {id2, {pathIds[0], pathIds[1], pathIds[3]}},
                           {id3, {pathIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.updateIdPaths({{id1, {pathIds[1]}}, {id2, {pathIds[3]}}});
}

TEST_F(ProjectStoragePathWatcher, UpdateIdPathsAndRemoveUnusedPaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[1]}}});

    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ProjectStoragePathWatcher, ExtractSortedEntriesFromConvertIdPaths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds({{id2, {pathIds[0], pathIds[1]}}, {id1, {pathIds[0], pathIds[1]}}});

    ASSERT_THAT(entriesAndIds.first,
                ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, ExtractSortedIdsFromConvertIdPaths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds({{id2, {}}, {id1, {}}, {id3, {}}});

    ASSERT_THAT(entriesAndIds.second, ElementsAre(ids[0], ids[1], ids[2]));
}

TEST_F(ProjectStoragePathWatcher, MergeEntries)
{
    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, MergeMoreEntries)
{
    watcher.updateIdPaths({{id2, {pathIds[0], pathIds[1]}}});

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ProjectStoragePathWatcher, AddEmptyEntries)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_))
            .Times(0);

    watcher.updateIdPaths({});
}

TEST_F(ProjectStoragePathWatcher, AddEntriesWithSameIdAndDifferentPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(ElementsAre(sourceContextPath, sourceContextPath2, sourceContextPath3)));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, AddEntriesWithDifferentIdAndSamePaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(ElementsAre(sourceContextPath)));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}});
}

TEST_F(ProjectStoragePathWatcher, DontAddNewEntriesWithSameIdAndSamePaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, DontAddNewEntriesWithDifferentIdAndSamePaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths({{id2, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});
}

TEST_F(ProjectStoragePathWatcher, RemoveEntriesWithId)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}},
                           {id2, {pathIds[0], pathIds[1]}},
                           {id3, {pathIds[1], pathIds[3]}}});

    watcher.removeIds({ProjectPartId{2}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry5, watcherEntry8));
}

TEST_F(ProjectStoragePathWatcher, RemoveNoPathsForEmptyIds)
{
    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({});
}

TEST_F(ProjectStoragePathWatcher, RemoveNoPathsForOneId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({id3.id});
}

TEST_F(ProjectStoragePathWatcher, RemovePathForOneId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id3, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(sourceContextPath2)));

    watcher.removeIds({id3.id});
}

TEST_F(ProjectStoragePathWatcher, RemoveNoPathSecondTime)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    watcher.removeIds({id2.id});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.removeIds({id2.id});
}

TEST_F(ProjectStoragePathWatcher, RemoveAllPathsForThreeId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher,
                removePaths(ElementsAre(sourceContextPath, sourceContextPath2)));

    watcher.removeIds({id1.id, id2.id, id3.id});
}

TEST_F(ProjectStoragePathWatcher, RemoveOnePathForTwoId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(sourceContextPath)));

    watcher.removeIds({id1.id, id2.id});
}

TEST_F(ProjectStoragePathWatcher, NotAnymoreWatchedEntriesWithId)
{
    watcher.addEntries(sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    auto oldEntries = watcher.notAnymoreWatchedEntriesWithIds({watcherEntry1, watcherEntry4}, {ids[0], ids[1]});

    ASSERT_THAT(oldEntries, ElementsAre(watcherEntry2, watcherEntry3));
}

TEST_F(ProjectStoragePathWatcher, RemoveUnusedEntries)
{
    watcher.addEntries(sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    watcher.removeFromWatchedEntries({watcherEntry2, watcherEntry3});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ProjectStoragePathWatcher, TwoNotifyFileChanges)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2]}},
                           {id2, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}},
                           {id3, {pathIds[4]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[0])))
        .WillByDefault(Return(FileStatus{pathIds[0], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[1])))
        .WillByDefault(Return(FileStatus{pathIds[1], 1, 2}));
    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[3])))
        .WillByDefault(Return(FileStatus{pathIds[3], 1, 2}));

    EXPECT_CALL(notifier,
                pathsWithIdsChanged(
                    ElementsAre(IdPaths{id1, {SourceId{1}, SourceId{2}}},
                                IdPaths{id2, {SourceId{1}, SourceId{2}, SourceId{4}}})));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
    mockQFileSytemWatcher.directoryChanged(sourceContextPath2);
}

TEST_F(ProjectStoragePathWatcher, NotifyForPathChanges)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[0])))
        .WillByDefault(Return(FileStatus{pathIds[0], 1, 2}));

    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[3])))
        .WillByDefault(Return(FileStatus{pathIds[3], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(pathIds[0])));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}

TEST_F(ProjectStoragePathWatcher, NoNotifyForUnwatchedPathChanges)
{
    watcher.updateIdPaths({{id1, {pathIds[3]}}, {id2, {pathIds[3]}}});

    EXPECT_CALL(notifier, pathsChanged(IsEmpty()));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}

TEST_F(ProjectStoragePathWatcher, NoDuplicatePathChanges)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    ON_CALL(mockFileSystem, fileStatus(Eq(pathIds[0])))
        .WillByDefault(Return(FileStatus{pathIds[0], 1, 2}));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(pathIds[0])));

    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
    mockQFileSytemWatcher.directoryChanged(sourceContextPath);
}
} // namespace
