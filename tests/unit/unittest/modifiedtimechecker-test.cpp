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

    ClangBackEnd::FilePathId id(const Utils::SmallStringView &path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{path});
    }


    ModifiedTimeChecker()
    {
        ON_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path1")))).WillByDefault(Return(50));
        ON_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path2")))).WillByDefault(Return(30));
    }
    NiceMock<MockFunction<ClangBackEnd::TimeStamp(ClangBackEnd::FilePathView filePath)>> getModifiedTimeCallback;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    decltype(getModifiedTimeCallback.AsStdFunction()) callback = getModifiedTimeCallback
                                                                     .AsStdFunction();
    ClangBackEnd::ModifiedTimeChecker checker{callback, filePathCache};
    SourceEntries upToDateEntries = {{id("/path1"), SourceType::UserInclude, 100},
                                     {id("/path2"), SourceType::SystemInclude, 30}};
    SourceEntries notUpToDateEntries = {{id("/path1"), SourceType::UserInclude, 50},
                                        {id("/path2"), SourceType::SystemInclude, 20}};
};

TEST_F(ModifiedTimeChecker, IsUpToDate)
{
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
    auto isUpToDate = checker.isUpToDate({});

    ASSERT_FALSE(isUpToDate);
}

TEST_F(ModifiedTimeChecker, PathChangesUpdatesTimeStamps)
{
    checker.isUpToDate(upToDateEntries);

    EXPECT_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path1"))));
    EXPECT_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path2"))));

    checker.pathsChanged({id(FilePathView("/path1")), id(FilePathView("/path2")), id(FilePathView("/path3"))});
}

TEST_F(ModifiedTimeChecker, IsNotUpToDateAnyMoreAfterUpdating)
{
    checker.isUpToDate(upToDateEntries);
    ON_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path1")))).WillByDefault(Return(120));
    ON_CALL(getModifiedTimeCallback, Call(Eq(FilePathView("/path2")))).WillByDefault(Return(30));

    checker.pathsChanged({id(FilePathView("/path1")), id(FilePathView("/path2")), id(FilePathView("/path3"))});

    ASSERT_FALSE(checker.isUpToDate(upToDateEntries));
}

} // namespace
