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

using StatementFactory = ClangBackEnd::FilePathStorageSqliteStatementFactory<NiceMock<MockSqliteDatabase>>;

class FilePathStorageSqliteStatementFactory : public testing::Test
{
protected:
    NiceMock<MockSqliteDatabase> mockDatabase;
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

TEST_F(FilePathStorageSqliteStatementFactory, SelectSourceNameAndDirectoryIdFromSourcesByAndSourceId)
{
    ASSERT_THAT(factory.selectSourceNameAndDirectoryIdFromSourcesBySourceId.sqlStatement,
                Eq("SELECT sourceName, directoryId FROM sources WHERE sourceId = ?"));
}

TEST_F(FilePathStorageSqliteStatementFactory, SelectSourceNameAndDirectoryIdBySourceId)
{
    ASSERT_THAT(factory.selectSourceNameAndDirectoryIdFromSourcesBySourceId.sqlStatement,
                Eq("SELECT sourceName, directoryId FROM sources WHERE sourceId = ?"));
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

}
