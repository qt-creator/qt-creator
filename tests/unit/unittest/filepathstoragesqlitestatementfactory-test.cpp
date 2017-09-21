/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "mockmutex.h"
#include "mocksqlitedatabase.h"
#include "mocksqlitereadstatement.h"
#include "mocksqlitewritestatement.h"

#include <filepathstoragesqlitestatementfactory.h>

namespace {

using StatementFactory = ClangBackEnd::FilePathStorageSqliteStatementFactory<NiceMock<MockSqliteDatabase>,
                                                                             MockSqliteReadStatement,
                                                                             MockSqliteWriteStatement>;

class FilePathStorageSqliteStatementFactory : public testing::Test
{
protected:
    NiceMock<MockMutex> mockMutex;
    NiceMock<MockSqliteDatabase> mockDatabase{mockMutex};
    StatementFactory factory{mockDatabase};
};

TEST_F(FilePathStorageSqliteStatementFactory, SelectDirectoryIdFromDirectoriesByDirectoryPath)
{
    ASSERT_THAT(factory.selectDirectoryIdFromDirectoriesByDirectoryPath.sqlStatement,
                Eq("SELECT directoryId FROM directories WHERE directoryPath = ?"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectDirectoryPathFromDirectoriesByDirectoryId)
{
    ASSERT_THAT(factory.selectDirectoryPathFromDirectoriesByDirectoryId.sqlStatement,
                Eq("SELECT directoryPath FROM directories WHERE directoryId = ?"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectSourceIdFromSourcesByDirectoryIdAndSourceName)
{
    ASSERT_THAT(factory.selectSourceIdFromSourcesByDirectoryIdAndSourceName.sqlStatement,
                Eq("SELECT sourceId FROM sources WHERE directoryId = ? AND sourceName = ?"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectSourceNameFromSourcesByDirectoryIdAndSourceId)
{
    ASSERT_THAT(factory.selectSourceNameFromSourcesBySourceId.sqlStatement,
                Eq("SELECT sourceName FROM sources WHERE sourceId = ?"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectAllDirectories)
{
    ASSERT_THAT(factory.selectAllDirectories.sqlStatement,
                Eq("SELECT directoryId, directoryPath FROM directories"));
}

TEST_F(FilePathStorageSqliteStatementFactory, InsertIntoDirectories)
{
    ASSERT_THAT(factory.insertIntoDirectories.sqlStatement,
                Eq("INSERT INTO directories(directoryPath) VALUES (?)"));
}

TEST_F(FilePathStorageSqliteStatementFactory, InsertIntoSources)
{
    ASSERT_THAT(factory.insertIntoSources.sqlStatement,
                Eq("INSERT INTO sources(directoryId, sourceName) VALUES (?,?)"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectAllSources)
{
    ASSERT_THAT(factory.selectAllSources.sqlStatement,
                Eq("SELECT sourceId, sourceName FROM sources"));
}

}
