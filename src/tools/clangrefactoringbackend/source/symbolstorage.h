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

#include "symbolstorageinterface.h"

#include <sqliteexception.h>
#include <sqlitetransaction.h>
#include <filepathcachingfwd.h>

namespace ClangBackEnd {

template <typename StatementFactory>
class SymbolStorage : public SymbolStorageInterface
{
    using Transaction = Sqlite::ImmediateTransaction<typename StatementFactory::DatabaseType>;
    using ReadStatement = typename StatementFactory::ReadStatementType;
    using WriteStatement = typename StatementFactory::WriteStatementType;
    using Database = typename StatementFactory::DatabaseType;

public:
    SymbolStorage(StatementFactory &statementFactory,
                  FilePathCachingInterface &filePathCache)
        : m_statementFactory(statementFactory),
          m_filePathCache(filePathCache)
    {
    }

    void addSymbolsAndSourceLocations(const SymbolEntries &symbolEntries,
                                      const SourceLocationEntries &sourceLocations) override
    {
        Transaction transaction{m_statementFactory.database};

        fillTemporarySymbolsTable(symbolEntries);
        fillTemporaryLocationsTable(sourceLocations);
        addNewSymbolsToSymbols();
        syncNewSymbolsFromSymbols();
        syncSymbolsIntoNewLocations();
        deleteAllLocationsFromUpdatedFiles();
        insertNewLocationsInLocations();
        deleteNewSymbolsTable();
        deleteNewLocationsTable();

        transaction.commit();
    }

    void fillTemporarySymbolsTable(const SymbolEntries &symbolEntries)
    {
        WriteStatement &statement = m_statementFactory.insertSymbolsToNewSymbolsStatement;

        for (const auto &symbolEntry : symbolEntries) {
            statement.write(symbolEntry.first,
                            symbolEntry.second.usr,
                            symbolEntry.second.symbolName);
        }
    }

    void fillTemporaryLocationsTable(const SourceLocationEntries &sourceLocations)
    {
        WriteStatement &statement = m_statementFactory.insertLocationsToNewLocationsStatement;

        for (const auto &locationsEntry : sourceLocations) {
            statement.write(locationsEntry.symbolId,
                            locationsEntry.line,
                            locationsEntry.column,
                            locationsEntry.filePathId.fileNameId);
        }
    }

    void addNewSymbolsToSymbols()
    {
        m_statementFactory.addNewSymbolsToSymbolsStatement.execute();
    }

    void syncNewSymbolsFromSymbols()
    {
        m_statementFactory.syncNewSymbolsFromSymbolsStatement.execute();
    }

    void syncSymbolsIntoNewLocations()
    {
        m_statementFactory.syncSymbolsIntoNewLocationsStatement.execute();
    }

    void deleteAllLocationsFromUpdatedFiles()
    {
        m_statementFactory.deleteAllLocationsFromUpdatedFilesStatement.execute();
    }

    void insertNewLocationsInLocations()
    {
        m_statementFactory.insertNewLocationsInLocationsStatement.execute();
    }

    void deleteNewSymbolsTable()
    {
        m_statementFactory.deleteNewSymbolsTableStatement.execute();
    }

    void deleteNewLocationsTable()
    {
        m_statementFactory.deleteNewLocationsTableStatement.execute();
    }

    SourceLocationEntries sourceLocations() const
    {
        return SourceLocationEntries();
    }

private:
    StatementFactory &m_statementFactory;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
