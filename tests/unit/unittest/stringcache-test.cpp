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

using testing::ElementsAre;

class StringCache : public testing::Test
{
protected:
    ClangBackEnd::StringCache<Utils::PathString> cache;
    Utils::PathString filePath1{"/file/pathOne"};
    Utils::PathString filePath2{"/file/pathTwo"};
    Utils::PathString filePath3{"/file/pathThree"};
    Utils::PathString filePath4{"/file/pathFour"};
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

}
