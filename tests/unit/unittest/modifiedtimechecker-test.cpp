/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#include "mockfilesystem.h"

#include <filepathcaching.h>
#include <modifiedtimechecker.h>
#include <refactoringdatabaseinitializer.h>

namespace {

using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceType;
using ClangBackEnd::FilePathView;


class ModifiedTimeChecker : public testing::Test
{
protected:
    ModifiedTimeChecker()
    {
        ON_CALL(mockFileSystem, lastModified(Eq(1))).WillByDefault(Return(50));
        ON_CALL(mockFileSystem, lastModified(Eq(2))).WillByDefault(Return(30));
        ON_CALL(mockFileSystem, lastModified(Eq(3))).WillByDefault(Return(50));
        ON_CALL(mockFileSystem, lastModified(Eq(4))).WillByDefault(Return(30));
    }

    NiceMock<MockFileSystem> mockFileSystem;
    ClangBackEnd::ModifiedTimeChecker<> checker{mockFileSystem};
    SourceEntries upToDateEntries = {{1, SourceType::UserInclude, 51},
                                     {2, SourceType::SystemInclude, 30},
                                     {3, SourceType::UserInclude, 50},
                                     {4, SourceType::SystemInclude, 31}};
    SourceEntries equalEntries = {{1, SourceType::UserInclude, 50},
                                  {2, SourceType::SystemInclude, 30},
                                  {3, SourceType::UserInclude, 50},
                                  {4, SourceType::SystemInclude, 30}};
    SourceEntries notUpToDateEntries = {{1, SourceType::UserInclude, 50},
                                        {2, SourceType::SystemInclude, 29},
                                        {3, SourceType::UserInclude, 50},
                                        {4, SourceType::SystemInclude, 30}};
};

TEST_F(ModifiedTimeChecker, IsUpToDate)
{
    auto isUpToDate = checker.isUpToDate(upToDateEntries);

    ASSERT_TRUE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, EqualEntriesAreUpToDate)
{
    auto isUpToDate = checker.isUpToDate(equalEntries);

    ASSERT_TRUE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, IsUpToDateSecondRun)
{
    checker.isUpToDate(upToDateEntries);

    auto isUpToDate = checker.isUpToDate(upToDateEntries);

    ASSERT_TRUE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, IsNotUpToDateIfSourceEntriesAreEmpty)
{
    auto isUpToDate = checker.isUpToDate({});

    ASSERT_FALSE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, IsNotUpToDate)
{
    auto isUpToDate = checker.isUpToDate(notUpToDateEntries);

    ASSERT_FALSE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, IsNotUpToDateSecondRun)
{
    checker.isUpToDate(notUpToDateEntries);

    auto isUpToDate = checker.isUpToDate(notUpToDateEntries);

    ASSERT_FALSE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, PathChangesUpdatesTimeStamps)
{
    checker.isUpToDate(upToDateEntries);

    EXPECT_CALL(mockFileSystem, lastModified(Eq(2)));
    EXPECT_CALL(mockFileSystem, lastModified(Eq(3)));

    checker.pathsChanged({2, 3});
}

TEST_F(ModifiedTimeChecker, IsNotUpToDateAnyMoreAfterUpdating)
{
    checker.isUpToDate(upToDateEntries);
    ON_CALL(mockFileSystem, lastModified(Eq(1))).WillByDefault(Return(120));
    ON_CALL(mockFileSystem, lastModified(Eq(2))).WillByDefault(Return(30));

    checker.pathsChanged({1, 2, 3});

    ASSERT_FALSE(checker.isUpToDate(upToDateEntries));
}

} // namespace
