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

#pragma once

#include <sqlitetransaction.h>
#include <sqlitetable.h>

namespace ClangBackEnd {

template<typename Database,
         typename ReadStatement,
         typename WriteStatement>
class FilePathStorageSqliteStatementFactory
{
public:
    using DatabaseType = Database;
    using ReadStatementType = ReadStatement;
    using WriteStatementType = WriteStatement;

    FilePathStorageSqliteStatementFactory(Database &database)
        : database(database)
    {
    }

public:
    Database &database;
    ReadStatement selectDirectoryIdFromDirectoriesByDirectoryPath{
      "SELECT directoryId FROM directories WHERE directoryPath = ?",
        database
    };
    ReadStatement selectDirectoryPathFromDirectoriesByDirectoryId{
      "SELECT directoryPath FROM directories WHERE directoryId = ?",
        database
    };
    ReadStatement selectAllDirectories{
      "SELECT directoryId, directoryPath FROM directories",
        database
    };
    WriteStatement insertIntoDirectories{
      "INSERT INTO directories(directoryPath) VALUES (?)",
        database
    };
    ReadStatement selectSourceIdFromSourcesByDirectoryIdAndSourceName{
      "SELECT sourceId FROM sources WHERE directoryId = ? AND sourceName = ?",
        database
    };
    ReadStatement selectSourceNameFromSourcesBySourceId{
      "SELECT sourceName FROM sources WHERE sourceId = ?",
        database
    };
    WriteStatement insertIntoSources{
      "INSERT INTO sources(directoryId, sourceName) VALUES (?,?)",
        database
    };
    ReadStatement selectAllSources{
      "SELECT sourceId, sourceName FROM sources",
        database
    };
};

} // namespace ClangBackEnd

