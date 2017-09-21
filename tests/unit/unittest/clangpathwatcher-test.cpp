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

#include "faketimer.h"
#include "mockfilepathcaching.h"
#include "mockqfilesystemwatcher.h"
#include "mockclangpathwatchernotifier.h"

#include <clangpathwatcher.h>

#include <utils/smallstring.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;
using testing::NiceMock;

using Watcher = ClangBackEnd::ClangPathWatcher<NiceMock<MockQFileSytemWatcher>, FakeTimer>;
using ClangBackEnd::WatcherEntry;
using ClangBackEnd::FilePath;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;

class ClangPathWatcher : public testing::Test
{
protected:
    void SetUp();

protected:
    NiceMock<MockFilePathCaching> filePathCache;
    NiceMock<MockClangPathWatcherNotifier> notifier;
    Watcher watcher{filePathCache, &notifier};
    NiceMock<MockQFileSytemWatcher> &mockQFileSytemWatcher = watcher.fileSystemWatcher();
    Utils::SmallString id1{"id4"};
    Utils::SmallString id2{"id2"};
    Utils::SmallString id3{"id3"};
    FilePath path1{Utils::PathString{"/path/path1"}};
    FilePath path2{Utils::PathString{"/path/path2"}};
    QString path1QString = QString(path1.path());
    QString path2QString = QString(path2.path());
    FilePathIds pathIds = {{1, 1}, {1, 2}};
    std::vector<int> ids{watcher.idCache().stringIds({id1, id2, id3})};
    WatcherEntry watcherEntry1{ids[0], pathIds[0]};
    WatcherEntry watcherEntry2{ids[1], pathIds[0]};
    WatcherEntry watcherEntry3{ids[0], pathIds[1]};
    WatcherEntry watcherEntry4{ids[1], pathIds[1]};
    WatcherEntry watcherEntry5{ids[2], pathIds[1]};
};

TEST_F(ClangPathWatcher, ConvertWatcherEntriesToQStringList)
{
    auto convertedList = watcher.convertWatcherEntriesToQStringList({watcherEntry1, watcherEntry3});

    ASSERT_THAT(convertedList, ElementsAre(path1QString, path2QString));
}

TEST_F(ClangPathWatcher, UniquePaths)
{
    auto uniqueEntries = watcher.uniquePaths({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4});

    ASSERT_THAT(uniqueEntries, ElementsAre(watcherEntry1, watcherEntry3));
}

TEST_F(ClangPathWatcher, NotWatchedEntries)
{
    watcher.addEntries({watcherEntry1, watcherEntry4});

    auto newEntries = watcher.notWatchedEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4});

    ASSERT_THAT(newEntries, ElementsAre(watcherEntry2, watcherEntry3));
}

TEST_F(ClangPathWatcher, AddIdPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1QString, path2QString}));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsCallsAddPathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path2QString}));

    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2QString}));

    watcher.updateIdPaths({{id1, {pathIds[0]}}, {id2, {pathIds[0]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsDoNotCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {pathIds[0], pathIds[1]}}, {id2, {pathIds[0], pathIds[1]}}, {id3, {pathIds[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2QString}))
            .Times(0);

    watcher.updateIdPaths({{id1, {pathIds[1]}}, {id2, {pathIds[0]}}});
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

TEST_F(ClangPathWatcher, NotWatchedPaths)
{
    watcher.mergeToWatchedEntries({watcherEntry1});

    auto newEntries = watcher.notWatchedPaths({watcherEntry2, watcherEntry3});

    ASSERT_THAT(newEntries, ElementsAre(watcherEntry3));
}

TEST_F(ClangPathWatcher, AddedPaths)
{
    watcher.mergeToWatchedEntries({watcherEntry1, watcherEntry2});

    auto filteredEntries = watcher.filterNotWatchedPaths({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4});

    ASSERT_THAT(filteredEntries, ElementsAre(watcherEntry3));
}

TEST_F(ClangPathWatcher, MergeEntries)
{
    watcher.mergeToWatchedEntries({watcherEntry1, watcherEntry4});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4));
}

TEST_F(ClangPathWatcher, MergeMoreEntries)
{
    watcher.mergeToWatchedEntries({watcherEntry1, watcherEntry4});

    watcher.mergeToWatchedEntries({watcherEntry2, watcherEntry3});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4));
}

TEST_F(ClangPathWatcher, AddEmptyEntries)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(_))
            .Times(0);

    watcher.addEntries({});
}

TEST_F(ClangPathWatcher, AddEntriesWithSameIdAndDifferentPaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1QString, path2QString}));

    watcher.addEntries({watcherEntry1, watcherEntry3});
}

TEST_F(ClangPathWatcher, AddEntriesWithDifferentIdAndSamePaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1QString}));

    watcher.addEntries({watcherEntry1, watcherEntry2});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithSameIdAndSamePaths)
{
    watcher.addEntries({watcherEntry1});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1QString}))
            .Times(0);

    watcher.addEntries({watcherEntry1});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithDifferentIdAndSamePaths)
{
    watcher.addEntries({watcherEntry1});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1QString}))
            .Times(0);

    watcher.addEntries({watcherEntry2});
}

TEST_F(ClangPathWatcher, RemoveEntriesWithId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    watcher.removeIdsFromWatchedEntries({ids[0]});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry2, watcherEntry4, watcherEntry5));
}

TEST_F(ClangPathWatcher, RemoveNoPathsForEmptyIds)
{
    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({});
}

TEST_F(ClangPathWatcher, RemoveNoPathsForOneId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(_))
            .Times(0);

    watcher.removeIds({id1});
}

TEST_F(ClangPathWatcher, RemovePathForOneId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2QString}));

    watcher.removeIds({id1});
}

TEST_F(ClangPathWatcher, RemoveAllPathsForThreeId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path1QString, path2QString}));

    watcher.removeIds({id1, id2, id3});
}

TEST_F(ClangPathWatcher, RemoveOnePathForTwoId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path1QString}));

    watcher.removeIds({id1, id2});
}

TEST_F(ClangPathWatcher, NotAnymoreWatchedEntriesWithId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    auto oldEntries = watcher.notAnymoreWatchedEntriesWithIds({watcherEntry1, watcherEntry4}, {ids[0], ids[1]});

    ASSERT_THAT(oldEntries, ElementsAre(watcherEntry2, watcherEntry3));
}

TEST_F(ClangPathWatcher, RemoveUnusedEntries)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    watcher.removeFromWatchedEntries({watcherEntry2, watcherEntry3});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ClangPathWatcher, EmptyVectorNotifyFileChange)
{
    watcher.addEntries({watcherEntry3});

    EXPECT_CALL(notifier, pathsWithIdsChanged(IsEmpty()));

    mockQFileSytemWatcher.fileChanged(path1QString);
}

TEST_F(ClangPathWatcher, NotifyFileChange)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(notifier, pathsWithIdsChanged(ElementsAre(id2, id1)));

    mockQFileSytemWatcher.fileChanged(path1QString);
}

TEST_F(ClangPathWatcher, TwoNotifyFileChanges)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(notifier, pathsWithIdsChanged(ElementsAre(id2, id3, id1)));

    mockQFileSytemWatcher.fileChanged(path2QString);
    mockQFileSytemWatcher.fileChanged(path1QString);
}

void ClangPathWatcher::SetUp()
{
    ON_CALL(filePathCache, filePathId(Eq(path1)))
            .WillByDefault(Return(pathIds[0]));
    ON_CALL(filePathCache, filePathId(Eq(path2)))
            .WillByDefault(Return(pathIds[1]));
    ON_CALL(filePathCache, filePath(pathIds[0]))
            .WillByDefault(Return(path1));
    ON_CALL(filePathCache, filePath(Eq(pathIds[1])))
            .WillByDefault(Return(path2));
}
}
