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
#include "mockqfilesystemwatcher.h"
#include "mockclangpathwatchernotifier.h"

#include <clangpathwatcher.h>
#include <stringcache.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;
using testing::NiceMock;

using Watcher = ClangBackEnd::ClangPathWatcher<NiceMock<MockQFileSytemWatcher>, FakeTimer>;
using ClangBackEnd::WatcherEntry;

class ClangPathWatcher : public testing::Test
{
protected:
    ClangBackEnd::StringCache<Utils::PathString> pathCache;
    NiceMock<MockClangPathWatcherNotifier> notifier;
    Watcher watcher{pathCache, &notifier};
    NiceMock<MockQFileSytemWatcher> &mockQFileSytemWatcher = watcher.fileSystemWatcher();
    Utils::SmallString id1{"id4"};
    Utils::SmallString id2{"id2"};
    Utils::SmallString id3{"id3"};
    Utils::PathString path1{"/path/path1"};
    Utils::PathString path2{"/path/path2"};
    std::vector<uint> paths = watcher.pathCache().stringIds({path1, path2});
    std::vector<uint> ids = watcher.idCache().stringIds({id1, id2, id3});
    WatcherEntry watcherEntry1{ids[0], paths[0]};
    WatcherEntry watcherEntry2{ids[1], paths[0]};
    WatcherEntry watcherEntry3{ids[0], paths[1]};
    WatcherEntry watcherEntry4{ids[1], paths[1]};
    WatcherEntry watcherEntry5{ids[2], paths[1]};
};

TEST_F(ClangPathWatcher, ConvertWatcherEntriesToQStringList)
{
    auto convertedList = watcher.convertWatcherEntriesToQStringList({watcherEntry1, watcherEntry3});

    ASSERT_THAT(convertedList, ElementsAre(path1, path2));
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
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1, path2}));

    watcher.updateIdPaths({{id1, {paths[0], paths[1]}}, {id2, {paths[0], paths[1]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsCallsAddPathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {paths[0]}}, {id2, {paths[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path2}));

    watcher.updateIdPaths({{id1, {paths[0], paths[1]}}, {id2, {paths[0], paths[1]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {paths[0], paths[1]}}, {id2, {paths[0], paths[1]}}, {id3, {paths[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2}));

    watcher.updateIdPaths({{id1, {paths[0]}}, {id2, {paths[0]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPathsDoNotCallsRemovePathInFileWatcher)
{
    watcher.updateIdPaths({{id1, {paths[0], paths[1]}}, {id2, {paths[0], paths[1]}}, {id3, {paths[0]}}});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2}))
            .Times(0);

    watcher.updateIdPaths({{id1, {paths[1]}}, {id2, {paths[0]}}});
}

TEST_F(ClangPathWatcher, UpdateIdPathsAndRemoveUnusedPaths)
{
    watcher.updateIdPaths({{id1, {paths[0], paths[1]}}, {id2, {paths[0], paths[1]}}, {id3, {paths[1]}}});

    watcher.updateIdPaths({{id1, {paths[0]}}, {id2, {paths[1]}}});

    ASSERT_THAT(watcher.watchedEntries(), ElementsAre(watcherEntry1, watcherEntry4, watcherEntry5));
}

TEST_F(ClangPathWatcher, ExtractSortedEntriesFromConvertIdPaths)
{
    auto entriesAndIds = watcher.convertIdPathsToWatcherEntriesAndIds({{id2, {paths[0], paths[1]}}, {id1, {paths[0], paths[1]}}});

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
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1, path2}));

    watcher.addEntries({watcherEntry1, watcherEntry3});
}

TEST_F(ClangPathWatcher, AddEntriesWithDifferentIdAndSamePaths)
{
    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1}));

    watcher.addEntries({watcherEntry1, watcherEntry2});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithSameIdAndSamePaths)
{
    watcher.addEntries({watcherEntry1});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1}))
            .Times(0);

    watcher.addEntries({watcherEntry1});
}

TEST_F(ClangPathWatcher, DontAddNewEntriesWithDifferentIdAndSamePaths)
{
    watcher.addEntries({watcherEntry1});

    EXPECT_CALL(mockQFileSytemWatcher, addPaths(QStringList{path1}))
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

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path2}));

    watcher.removeIds({id1});
}

TEST_F(ClangPathWatcher, RemoveAllPathsForThreeId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path1, path2}));

    watcher.removeIds({id1, id2, id3});
}

TEST_F(ClangPathWatcher, RemoveOnePathForTwoId)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(mockQFileSytemWatcher, removePaths(QStringList{path1}));

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

    EXPECT_CALL(notifier, pathsWithIdsChanged(IsEmpty())).Times(1);

    mockQFileSytemWatcher.fileChanged(path1.toQString());
}

TEST_F(ClangPathWatcher, NotifyFileChange)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(notifier, pathsWithIdsChanged(ElementsAre(id2, id1)));

    mockQFileSytemWatcher.fileChanged(path1.toQString());
}

TEST_F(ClangPathWatcher, TwoNotifyFileChanges)
{
    watcher.addEntries({watcherEntry1, watcherEntry2, watcherEntry3, watcherEntry4, watcherEntry5});

    EXPECT_CALL(notifier, pathsWithIdsChanged(ElementsAre(id2, id3, id1)));

    mockQFileSytemWatcher.fileChanged(path2.toQString());
    mockQFileSytemWatcher.fileChanged(path1.toQString());
}

}
