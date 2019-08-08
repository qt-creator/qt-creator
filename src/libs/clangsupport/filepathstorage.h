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
    using ReadStatement = typename StatementFactory::ReadStatement;
    using WriteStatement = typename StatementFactory::WriteStatement;
    using Database = typename StatementFactory::Database;

public:
    FilePathStorage(StatementFactory &statementFactory)
        : m_statementFactory(statementFactory)
    {}

    int fetchDirectoryIdUnguarded(Utils::SmallStringView directoryPath)
    {
        Utils::optional<int> optionalDirectoryId = readDirectoryId(directoryPath);

        if (optionalDirectoryId)
            return optionalDirectoryId.value();

        return writeDirectoryId(directoryPath);
    }

    int fetchDirectoryId(Utils::SmallStringView directoryPath)
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            Utils::optional<int> optionalDirectoryId = readDirectoryId(directoryPath);

            int directoryId = -1;

            if (optionalDirectoryId)
                directoryId = optionalDirectoryId.value();
            else
                directoryId = writeDirectoryId(directoryPath);

            transaction.commit();

            return directoryId;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchDirectoryId(directoryPath);
        } catch (const Sqlite::ConstraintPreventsModification &) {
            return fetchDirectoryId(directoryPath);
        }
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
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            ReadStatement &statement = m_statementFactory.selectDirectoryPathFromDirectoriesByDirectoryId;

            auto optionalDirectoryPath = statement.template value<Utils::PathString>(directoryPathId);

            if (!optionalDirectoryPath)
                throw DirectoryPathIdDoesNotExists();

            transaction.commit();

            return optionalDirectoryPath.value();
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchDirectoryPath(directoryPathId);
        }
    }

    std::vector<Sources::Directory> fetchAllDirectories()
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            ReadStatement &statement = m_statementFactory.selectAllDirectories;

            auto directories =  statement.template values<Sources::Directory, 2>(256);

            transaction.commit();

            return directories;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchAllDirectories();
        }
    }

    int fetchSourceIdUnguarded(int directoryId, Utils::SmallStringView sourceName)
    {
        Utils::optional<int> optionalSourceId = readSourceId(directoryId, sourceName);

        if (optionalSourceId)
            return optionalSourceId.value();

        return writeSourceId(directoryId, sourceName);
    }

    int fetchSourceId(int directoryId, Utils::SmallStringView sourceName)
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            int sourceId = fetchSourceIdUnguarded(directoryId, sourceName);

            transaction.commit();

            return sourceId;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchSourceId(directoryId, sourceName);
        } catch (const Sqlite::ConstraintPreventsModification &) {
            return fetchSourceId(directoryId, sourceName);
        }
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

    Sources::SourceNameAndDirectoryId fetchSourceNameAndDirectoryId(int sourceId)
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            ReadStatement &statement = m_statementFactory.selectSourceNameAndDirectoryIdFromSourcesBySourceId;

            auto optionalSourceName = statement.template value<Sources::SourceNameAndDirectoryId, 2>(sourceId);

            if (!optionalSourceName)
                throw SourceNameIdDoesNotExists();

            transaction.commit();

            return *optionalSourceName;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchSourceNameAndDirectoryId(sourceId);
        }
    }

    int fetchDirectoryId(int sourceId)
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            ReadStatement &statement = m_statementFactory.selectDirectoryIdFromSourcesBySourceId;

            auto optionalDirectoryId = statement.template value<int>(sourceId);

            if (!optionalDirectoryId)
                throw SourceNameIdDoesNotExists();

            transaction.commit();

            return *optionalDirectoryId;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchDirectoryId(sourceId);
        }
    }

    std::vector<Sources::Source> fetchAllSources()
    {
        try {
            Sqlite::DeferredTransaction transaction{m_statementFactory.database};

            ReadStatement &statement = m_statementFactory.selectAllSources;

            auto sources = statement.template values<Sources::Source, 3>(8192);

            transaction.commit();

            return sources;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchAllSources();
        }
    }

    Database &database() { return m_statementFactory.database; }

private:
    StatementFactory &m_statementFactory;
};

} // namespace ClangBackEnd
