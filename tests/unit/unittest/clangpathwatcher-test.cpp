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

#include "mockclangpathwatchernotifier.h"
#include "mockfilepathcaching.h"
#include "mockfilesystem.h"
#include "mockqfilesystemwatcher.h"
#include "mocktimer.h"

#include <clangpathwatcher.h>

#include <utils/smallstring.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;
using testing::NiceMock;

using Watcher = ClangBackEnd::ClangPathWatcher<NiceMock<MockQFileSytemWatcher>, NiceMock<MockTimer>>;
using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;
using ClangBackEnd::IdPaths;
using ClangBackEnd::ProjectChunkId;
using ClangBackEnd::ProjectPartId;
using ClangBackEnd::ProjectPartIds;
using ClangBackEnd::SourceType;
using ClangBackEnd::WatcherEntries;
using ClangBackEnd::WatcherEntry;

class ClangPathWatcher : public testing::Test
{
protected:
    void SetUp()
    {
        ON_CALL(mockFilePathCache, filePathId(Eq(path1))).WillByDefault(Return(pathIds[0]));
        ON_CALL(mockFilePathCache, filePathId(Eq(path2))).WillByDefault(Return(pathIds[1]));
        ON_CALL(mockFilePathCache, filePathId(Eq(path3))).WillByDefault(Return(pathIds[2]));
        ON_CALL(mockFilePathCache, filePathId(Eq(path4))).WillByDefault(Return(pathIds[3]));
        ON_CALL(mockFilePathCache, filePathId(Eq(path5))).WillByDefault(Return(pathIds[4]));
        ON_CALL(mockFilePathCache, filePath(Eq(pathIds[0]))).WillByDefault(Return(FilePath{path1}));
        ON_CALL(mockFilePathCache, filePath(Eq(pathIds[1]))).WillByDefault(Return(FilePath{path2}));
        ON_CALL(mockFilePathCache, filePath(Eq(pathIds[2]))).WillByDefault(Return(FilePath{path3}));
        ON_CALL(mockFilePathCache, filePath(Eq(pathIds[3]))).WillByDefault(Return(FilePath{path4}));
        ON_CALL(mockFilePathCache, filePath(Eq(pathIds[4]))).WillByDefault(Return(FilePath{path5}));
        ON_CALL(mockFilePathCache, directoryPathId(TypedEq<FilePathId>(pathIds[0])))
            .WillByDefault(Return(directoryPaths[0]));
        ON_CALL(mockFilePathCache, directoryPathId(TypedEq<FilePathId>(pathIds[1])))
            .WillByDefault(Return(directoryPaths[0]));
        ON_CALL(mockFilePathCache, directoryPathId(TypedEq<FilePathId>(pathIds[2])))
            .WillByDefault(Return(directoryPaths[1]));
        ON_CALL(mockFilePathCache, directoryPathId(TypedEq<FilePathId>(pathIds[3])))
            .WillByDefault(Return(directoryPaths[1]));
        ON_CALL(mockFilePathCache, directoryPathId(TypedEq<FilePathId>(pathIds[4])))
            .WillByDefault(Return(directoryPaths[2]));
        ON_CALL(mockFileSystem, lastModified(_)).WillByDefault(Return(1));
        ON_CALL(mockFilePathCache,
                directoryPathId(TypedEq<Utils::SmallStringView>(directoryPathString)))
            .WillByDefault(Return(directoryPaths[0]));
        ON_CALL(mockFilePathCache,
                directoryPathId(TypedEq<Utils::SmallStringView>(directoryPathString2)))
            .WillByDefault(Return(directoryPaths[1]));
        ON_CALL(mockFilePathCache, directoryPath(Eq(directoryPaths[0])))
            .WillByDefault(Return(directoryPath));
        ON_CALL(mockFilePathCache, directoryPath(Eq(directoryPaths[1])))
            .WillByDefault(Return(directoryPath2));
        ON_CALL(mockFilePathCache, directoryPath(Eq(directoryPaths[2])))
            .WillByDefault(Return(directoryPath3));
        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath)))
            .WillByDefault(Return(FilePathIds{pathIds[0], pathIds[1]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath2)))
            .WillByDefault(Return(FilePathIds{pathIds[2], pathIds[3]}));
        ON_CALL(mockFileSystem, directoryEntries(Eq(directoryPath3)))
            .WillByDefault(Return(FilePathIds{pathIds[4]}));
    }
    static WatcherEntries sorted(WatcherEntries &&entries)
    {
        std::stable_sort(entries.begin(), entries.end());

        return std::move(entries);
    }

protected:
    NiceMock<MockFilePathCaching> mockFilePathCache;
    NiceMock<MockClangPathWatcherNotifier> notifier;
    NiceMock<MockFileSystem> mockFileSystem;
    Watcher watcher{mockFilePathCache, mockFileSystem, &notifier};
    NiceMock<MockQFileSytemWatcher> &mockQFileSytemWatcher = watcher.fileSystemWatcher();
    ProjectChunkId id1{2, SourceType::ProjectInclude};
    ProjectChunkId id2{2, SourceType::Source};
    ProjectChunkId id3{4, SourceType::SystemInclude};
    FilePathView path1{"/path/path1"};
    FilePathView path2{"/path/path2"};
    FilePathView path3{"/path2/path1"};
    FilePathView path4{"/path2/path2"};
    FilePathView path5{"/path3/path"};
    QString path1QString = QString(path1.toStringView());
    QString path2QString = QString(path2.toStringView());
    QString directoryPath = "/path";
    QString directoryPath2 = "/path2";
    QString directoryPath3 = "/path3";
    Utils::PathString directoryPathString = directoryPath;
    Utils::PathString directoryPathString2 = directoryPath2;
    FilePathIds pathIds = {1, 2, 3, 4, 5};
    ClangBackEnd::DirectoryPathIds directoryPaths = {1, 2, 3};
    ClangBackEnd::ProjectChunkIds ids{id1, id2, id3};
    WatcherEntry watcherEntry1{id1, directoryPaths[0], pathIds[0]};
    WatcherEntry watcherEntry2{id2, directoryPaths[0], pathIds[0]};
    WatcherEntry watcherEntry3{id1, directoryPaths[0], pathIds[1]};
    WatcherEntry watcherEntry4{id2, directoryPaths[0], pathIds[1]};
    WatcherEntry watcherEntry5{id3, directoryPaths[0], pathIds[1]};
    WatcherEntry watcherEntry6{id1, directoryPaths[1], pathIds[2]};
    WatcherEntry watcherEntry7{id2, directoryPaths[1], pathIds[3]};
    WatcherEntry watcherEntry8{id3, directoryPaths[1], pathIds[3]};
};

TEST_F(ClangPathWatcher, AddIdPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(UnorderedElementsAre(QString(directoryPath), QString(directoryPath2))));

    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsCallsAddPathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(UnorderedElementsAre(QString(directoryPath2))));

    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(UnorderedElementsAre(QString(directoryPath2))));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsDoNotCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2]}},
                           {id2, {pathIds[0], pathIds[1], pathIds[3]}},
                           {id3, {pathIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.updateIdPaths({{id1, {pathIds[1]}}, {id2, {pathIds[3]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[1]}}});

    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ClangPathWatcher, ExtractSortedEntriesFromConvertIdPaths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds({{id2, {pathIds[0], pathIds[1]}}, {id1, {pathIds[0], pathIds[1]}}});

    ASSERT_THAT(entriesAndIds.first, ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ClangPathWatcher, ExtractSortedIdsFromConvertIdPaths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds({{id2, {}}, {id1, {}}, {id3, {}}});

    ASSERT_THAT(entriesAndIds.second, ElementsAre(ids[0], ids[1], ids[2]));
}

TEST_F(ClangPathWatcher, MergeEntries)
{
    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4));
}

TEST_F(ClangPathWatcher, MergeMoreEntries)
{
    watcher.updateIdPaths({{id2, {pathIds[0], pathIds[1]}}});

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ClangPathWatcher, AddEmptyEntries)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_))
            .Times(0);

    watcher.updateIdPaths({});
}

TEST_F(ClangPathWatcher, AddEntriesWithSameIdAndDifferentPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher,
                addPaths(ElementsAre(directoryPath, directoryPath2, directoryPath3)));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[4]}}});
}

TEST_F(ClangPathWatcher, AddEntriesWithDifferentIdAndSamePaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(ElementsAre(directoryPath)));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithSameIdAndSamePaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithDifferentIdAndSamePaths)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_)).Times(0);

    watcher.updateIdPaths({{id2, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}}});
}

TEST_F(ClangPathWatcher, RemoveEntriesWithId)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}},
                           {id2, {pathIds[0], pathIds[1]}},
                           {id3, {pathIds[1], pathIds[3]}}});

    watcher.removeIds({2});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry5, watcherEntry8));
}

TEST_F(ClangPathWatcher, RemoveNoPathsForEmptyIds)
{
    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({});
}

TEST_F(ClangPathWatcher, RemoveNoPathsForOneId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({id3.id});
}

TEST_F(ClangPathWatcher, RemovePathForOneId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id3, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(directoryPath2)));

    watcher.removeIds({id3.id});
}

TEST_F(ClangPathWatcher, RemoveNoPathSecondTime)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    watcher.removeIds({id2.id});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_)).Times(0);

    watcher.removeIds({id2.id});
}

TEST_F(ClangPathWatcher, RemoveAllPathsForThreeId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(directoryPath, directoryPath2)));

    watcher.removeIds({id1.id, id2.id, id3.id});
}

TEST_F(ClangPathWatcher, RemoveOnePathForTwoId)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[3]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(ElementsAre(directoryPath)));

    watcher.removeIds({id1.id, id2.id});
}

TEST_F(ClangPathWatcher, NotAnymoreWatchedEntriesWithId)
{
    watcher.addEntries(sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    auto oldEntries = watcher.notAnymoreWatchedEntriesWithIds({watcherEntry1, watcherEntry4}, {ids[0], ids[1]});

    ASSERT_THAT(oldEntries, ElementsAre(watcherEntry2, watcherEntry3));
}

TEST_F(ClangPathWatcher, RemoveUnusedEntries)
{
    watcher.addEntries(sorted({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5}));

    watcher.removeFromWatchedEntries({watcherEntry2, watcherEntry3});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ClangPathWatcher, TwoNotifyFileChanges)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1], pathIds[2]}},
                           {id2, {pathIds[0], pathIds[1], pathIds[2], pathIds[3], pathIds[4]}},
                           {id3, {pathIds[4]}}});
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[0]))).WillByDefault(Return(2));
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[1]))).WillByDefault(Return(2));
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[3]))).WillByDefault(Return(2));

    EXPECT_CALL(notifier,
                pathsWithIdsChanged(ElementsAre(IdPaths{id1, {1, 2}}, IdPaths{id2, {1, 2, 4}})));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
    mockQFileSytemWatcher.directoryChanged(directoryPath2);
}

TEST_F(ClangPathWatcher, NotifyForPathChanges)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[0]))).WillByDefault(Return(2));
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[3]))).WillByDefault(Return(2));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(pathIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ClangPathWatcher, NoNotifyForUnwatchedPathChanges)
{
    watcher.updateIdPaths({{id1, {pathIds[3]}}, {id2, {pathIds[3]}}});

    EXPECT_CALL(notifier, pathsChanged(IsEmpty()));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
}

TEST_F(ClangPathWatcher, NoDuplicatePathChanges)
{
    watcher.updateIdPaths(
        {{id1, {pathIds[0], pathIds[1], pathIds[2]}}, {id2, {pathIds[0], pathIds[1], pathIds[3]}}});
    ON_CALL(mockFileSystem, lastModified(Eq(pathIds[0]))).WillByDefault(Return(2));

    EXPECT_CALL(notifier, pathsChanged(ElementsAre(pathIds[0])));

    mockQFileSytemWatcher.directoryChanged(directoryPath);
    mockQFileSytemWatcher.directoryChanged(directoryPath);
}
} // namespace
