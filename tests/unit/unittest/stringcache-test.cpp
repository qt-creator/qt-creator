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

#include <stringcache.h>

#include <utils/smallstringio.h>

namespace {

using ClangBackEnd::StringCacheException;

using uint64 = unsigned long long;

using Utils::compare;
using Utils::reverseCompare;
using ClangBackEnd::findInSorted;

using CacheEntries = ClangBackEnd::FileCacheCacheEntries;

class StringCache : public testing::Test
{
protected:
    void SetUp();

protected:
    ClangBackEnd::FilePathCache<> cache;
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

TEST_F(StringCache, AddFilePath)
{
    auto id = cache.stringId(filePath1);

    ASSERT_THAT(id, 0);
}

TEST_F(StringCache, AddSecondFilePath)
{
    cache.stringId(filePath1);

    auto id = cache.stringId(filePath2);

    ASSERT_THAT(id, 1);
}

TEST_F(StringCache, AddDuplicateFilePath)
{
    cache.stringId(filePath1);

    auto id = cache.stringId(filePath1);

    ASSERT_THAT(id, 0);
}

TEST_F(StringCache, AddDuplicateFilePathBetweenOtherEntries)
{
    cache.stringId(filePath1);
    cache.stringId(filePath2);
    cache.stringId(filePath3);
    cache.stringId(filePath4);

    auto id = cache.stringId(filePath3);

    ASSERT_THAT(id, 2);
}

TEST_F(StringCache, ThrowForGettingPathForNoEntry)
{
    EXPECT_ANY_THROW(cache.string(0));
}

TEST_F(StringCache, GetFilePathForIdWithOneEntry)
{
    cache.stringId(filePath1);

    auto filePath = cache.string(0);

    ASSERT_THAT(filePath, filePath1);
}

TEST_F(StringCache, GetFilePathForIdWithSomeEntries)
{
    cache.stringId(filePath1);
    cache.stringId(filePath2);
    cache.stringId(filePath3);
    cache.stringId(filePath4);

    auto filePath = cache.string(2);

    ASSERT_THAT(filePath, filePath3);
}

TEST_F(StringCache, GetAllFilePaths)
{
    cache.stringId(filePath1);
    cache.stringId(filePath2);
    cache.stringId(filePath3);
    cache.stringId(filePath4);

    const auto &filePaths = cache.strings({0, 1, 2, 3});

    ASSERT_THAT(filePaths, ElementsAre(filePath1, filePath2, filePath3, filePath4));
}

TEST_F(StringCache, AddFilePaths)
{
    auto ids = cache.stringIds({filePath1, filePath2, filePath3, filePath4});

    ASSERT_THAT(ids, ElementsAre(0, 1, 2, 3));
}

TEST_F(StringCache, IsEmpty)
{
    auto isEmpty = cache.isEmpty();

    ASSERT_TRUE(isEmpty);
}

TEST_F(StringCache, IsNotEmpty)
{
    cache.stringId(filePath1);

    auto isEmpty = cache.isEmpty();

    ASSERT_FALSE(isEmpty);
}

TEST_F(StringCache, PopulateWithEmptyVector)
{
    CacheEntries entries;

    cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(cache.isEmpty());
}

TEST_F(StringCache, IsNotEmptyAfterPopulateWithSomeEntries)
{
    CacheEntries entries{{filePath1.clone(), 0},
                         {filePath2.clone(), 1},
                         {filePath3.clone(), 2},
                         {filePath4.clone(), 3}};

    cache.uncheckedPopulate(std::move(entries));

    ASSERT_TRUE(!cache.isEmpty());
}

TEST_F(StringCache, GetEntryAfterPopulateWithSomeEntries)
{
    CacheEntries entries{{filePath1.clone(), 0},
                         {filePath2.clone(), 1},
                         {filePath3.clone(), 2},
                         {filePath4.clone(), 3}};
    cache.uncheckedPopulate(std::move(entries));

    auto string = cache.string(2);

    ASSERT_THAT(string, filePath3);
}

TEST_F(StringCache, EntriesHaveUniqueIds)
{
    CacheEntries entries{{filePath1.clone(), 0},
                         {filePath2.clone(), 1},
                         {filePath3.clone(), 2},
                         {filePath4.clone(), 2}};

    ASSERT_THROW(cache.populate(std::move(entries)), StringCacheException);
}

TEST_F(StringCache, IdsAreHigherLowerEntriesSize)
{
    CacheEntries entries{{filePath1.clone(), 0},
                         {filePath2.clone(), 1},
                         {filePath3.clone(), 4},
                         {filePath4.clone(), 3}};

    ASSERT_THROW(cache.populate(std::move(entries)), std::out_of_range);
}

TEST_F(StringCache, MultipleEntries)
{
    CacheEntries entries{{filePath1.clone(), 0},
                         {filePath1.clone(), 1},
                         {filePath3.clone(), 2},
                         {filePath4.clone(), 3}};

    ASSERT_THROW(cache.populate(std::move(entries)), StringCacheException);
}

TEST_F(StringCache, DontFindInSorted)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathFoo", compare);

    ASSERT_FALSE(found.wasFound);
}

TEST_F(StringCache, FindInSortedOne)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathOne", compare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedTwo)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathTwo", compare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedTree)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathThree", compare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedFour)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathFour", compare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedFife)
{
    auto found = findInSorted(filePaths.cbegin(), filePaths.cend(), "/file/pathFife", compare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, DontFindInSortedReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathFoo", reverseCompare);

    ASSERT_FALSE(found.wasFound);
}

TEST_F(StringCache, FindInSortedOneReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathOne", reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedTwoReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathTwo", reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedTreeReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathThree", reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedFourReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathFour", reverseCompare);

    ASSERT_TRUE(found.wasFound);
}

TEST_F(StringCache, FindInSortedFifeReverse)
{
    auto found = findInSorted(reverseFilePaths.cbegin(), reverseFilePaths.cend(), "/file/pathFife", reverseCompare);

    ASSERT_TRUE(found.wasFound);
}


void StringCache::SetUp()
{
    std::sort(filePaths.begin(), filePaths.end(), [] (auto &f, auto &l) { return compare(f, l) < 0;});
    std::sort(reverseFilePaths.begin(), reverseFilePaths.end(), [] (auto &f, auto &l) { return reverseCompare(f, l) < 0;});
}
}

