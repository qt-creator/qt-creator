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

#include "filepathexceptions.h"
#include "filepathid.h"
#include "filepathstoragesources.h"

#include <sqliteexception.h>
#include <sqlitetransaction.h>

#include <utils/optional.h>

namespace ClangBackEnd {

template <typename StatementFactory>
class FilePathStorage
{
    using DeferredTransaction = Sqlite::DeferredTransaction<typename StatementFactory::DatabaseType>;
    using ImmediateTransaction = Sqlite::ImmediateTransaction<typename StatementFactory::DatabaseType>;
    using ReadStatement = typename StatementFactory::ReadStatementType;
    using WriteStatement = typename StatementFactory::WriteStatementType;
    using Database = typename StatementFactory::DatabaseType;

public:
    FilePathStorage(StatementFactory &statementFactory)
        : m_statementFactory(statementFactory)
    {}

    int fetchDirectoryId(Utils::SmallStringView directoryPath)
    try {
        DeferredTransaction transaction{m_statementFactory.database};

        Utils::optional<int> optionalDirectoryId = readDirectoryId(directoryPath);

        int directoryId = -1;

        if (optionalDirectoryId)
            directoryId = optionalDirectoryId.value();
        else
            directoryId = writeDirectoryId(directoryPath);

        transaction.commit();

        return directoryId;
    } catch (Sqlite::StatementIsBusy &) {
        return fetchDirectoryId(directoryPath);
    }

    Utils::optional<int> readDirectoryId(Utils::SmallStringView directoryPath)
    {
        ReadStatement &statement = m_statementFactory.selectDirectoryIdFromDirectoriesByDirectoryPath;

        return statement.template value<int>(directoryPath);
    }

    int writeDirectoryId(Utils::SmallStringView directoryPath)
    {
        WriteStatement &statement = m_statementFactory.insertIntoDirectories;

        statement.write(directoryPath);

        return int(m_statementFactory.database.lastInsertedRowId());
    }

    Utils::PathString fetchDirectoryPath(int directoryPathId)
    {
        DeferredTransaction transaction{m_statementFactory.database};

        ReadStatement &statement = m_statementFactory.selectDirectoryPathFromDirectoriesByDirectoryId;

        auto optionalDirectoryPath = statement.template value<Utils::PathString>(directoryPathId);

        if (!optionalDirectoryPath)
            throw DirectoryPathIdDoesNotExists();

        transaction.commit();

        return optionalDirectoryPath.value();
    }

    std::vector<Sources::Directory> fetchAllDirectories()
    {
        DeferredTransaction transaction{m_statementFactory.database};

        ReadStatement &statement = m_statementFactory.selectAllDirectories;

        auto directories =  statement.template values<Sources::Directory, 2>(256);

        transaction.commit();

        return directories;
    }

    int fetchSourceId(int directoryId, Utils::SmallStringView sourceName)
    try {
        DeferredTransaction transaction{m_statementFactory.database};

        Utils::optional<int> optionalSourceId = readSourceId(directoryId, sourceName);

        int sourceId = -1;

        if (optionalSourceId)
            sourceId = optionalSourceId.value();
        else
            sourceId = writeSourceId(directoryId, sourceName);

        transaction.commit();

        return sourceId;
    } catch (Sqlite::StatementIsBusy &) {
        return fetchSourceId(directoryId, sourceName);
    }

    int writeSourceId(int directoryId, Utils::SmallStringView sourceName)
    {
        WriteStatement &statement = m_statementFactory.insertIntoSources;

        statement.write(directoryId, sourceName);

        return int(m_statementFactory.database.lastInsertedRowId());
    }

    Utils::optional<int> readSourceId(int directoryId, Utils::SmallStringView sourceName)
    {
        ReadStatement &statement = m_statementFactory.selectSourceIdFromSourcesByDirectoryIdAndSourceName;

        return statement.template value<int>(directoryId, sourceName);
    }

    Utils::SmallString fetchSourceName(int sourceId)
    {
        DeferredTransaction transaction{m_statementFactory.database};

        ReadStatement &statement = m_statementFactory.selectSourceNameFromSourcesBySourceId;

        auto optionalSourceName = statement.template value<Utils::SmallString>(sourceId);

        if (!optionalSourceName)
            throw SourceNameIdDoesNotExists();

        transaction.commit();

        return optionalSourceName.value();
    }

    std::vector<Sources::Source> fetchAllSources()
    {
        DeferredTransaction transaction{m_statementFactory.database};

        ReadStatement &statement = m_statementFactory.selectAllSources;

        auto sources =  statement.template values<Sources::Source, 2>(8192);

        transaction.commit();

        return sources;
    }

private:
    StatementFactory &m_statementFactory;
};

} // namespace ClangBackEnd
