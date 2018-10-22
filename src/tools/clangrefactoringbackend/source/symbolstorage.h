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

#include <filepathcachingfwd.h>
#include <sqliteexception.h>
#include <sqlitetransaction.h>
#include <sqlitetable.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {

template <typename DatabaseType>
class SymbolStorage final : public SymbolStorageInterface
{
    using Database = DatabaseType;
    using ReadStatement = typename Database::ReadStatement;
    using WriteStatement = typename Database::WriteStatement;

public:
    SymbolStorage(Database &database)
        : m_transaction(database),
          m_database(database)
    {
        m_transaction.commit();
    }

    void addSymbolsAndSourceLocations(const SymbolEntries &symbolEntries,
                                      const SourceLocationEntries &sourceLocations) override
    {
        fillTemporarySymbolsTable(symbolEntries);
        fillTemporaryLocationsTable(sourceLocations);
        addNewSymbolsToSymbols();
        syncNewSymbolsFromSymbols();
        syncSymbolsIntoNewLocations();
        deleteAllLocationsFromUpdatedFiles();
        insertNewLocationsInLocations();
        deleteNewSymbolsTable();
        deleteNewLocationsTable();
    }

    int insertOrUpdateProjectPart(Utils::SmallStringView projectPartName,
                                   const Utils::SmallStringVector &commandLineArguments,
                                   const CompilerMacros &compilerMacros,
                                   const Utils::SmallStringVector &includeSearchPaths) override
    {
        m_database.setLastInsertedRowId(-1);

        Utils::SmallString compilerArguementsAsJson = toJson(commandLineArguments);
        Utils::SmallString compilerMacrosAsJson = toJson(compilerMacros);
        Utils::SmallString includeSearchPathsAsJason = toJson(includeSearchPaths);

        WriteStatement &insertStatement = m_insertProjectPartStatement;
        insertStatement.write(projectPartName,
                              compilerArguementsAsJson,
                              compilerMacrosAsJson,
                              includeSearchPathsAsJason);

        if (m_database.lastInsertedRowId() == -1) {
            WriteStatement &updateStatement = m_updateProjectPartStatement;
            updateStatement.write(compilerArguementsAsJson,
                                  compilerMacrosAsJson,
                                  includeSearchPathsAsJason,
                                  projectPartName);
        }

        return int(m_database.lastInsertedRowId());
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(FilePathId sourceId) const override
    {
        ReadStatement &statement = m_getProjectPartArtefactsBySourceId;

        return statement.template value<ProjectPartArtefact, 4>(sourceId.filePathId);
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(Utils::SmallStringView projectPartName) const override
    {
        ReadStatement &statement = m_getProjectPartArtefactsByProjectPartName;

        return statement.template value<ProjectPartArtefact, 4>(projectPartName);
    }

    void updateProjectPartSources(int projectPartId,
                                  const FilePathIds &sourceFilePathIds) override
    {
        WriteStatement &deleteStatement = m_deleteAllProjectPartsSourcesWithProjectPartIdStatement;
        deleteStatement.write(projectPartId);

        WriteStatement &insertStatement = m_insertProjectPartSourcesStatement;
        for (const FilePathId &sourceFilePathId : sourceFilePathIds)
            insertStatement.write(projectPartId, sourceFilePathId.filePathId);
    }

    static Utils::SmallString toJson(const Utils::SmallStringVector &strings)
    {
        QJsonDocument document;
        QJsonArray array;

        std::transform(strings.begin(), strings.end(), std::back_inserter(array), [] (const auto &string) {
            return QJsonValue(string.data());
        });

        document.setArray(array);

        return document.toJson(QJsonDocument::Compact);
    }

    static Utils::SmallString toJson(const CompilerMacros &compilerMacros)
    {
        QJsonDocument document;
        QJsonObject object;

        for (const CompilerMacro &macro : compilerMacros)
            object.insert(QString(macro.key), QString(macro.value));

        document.setObject(object);

        return document.toJson(QJsonDocument::Compact);
    }

    void fillTemporarySymbolsTable(const SymbolEntries &symbolEntries)
    {
        WriteStatement &statement = m_insertSymbolsToNewSymbolsStatement;

        for (const auto &symbolEntry : symbolEntries) {
            statement.write(symbolEntry.first,
                            symbolEntry.second.usr,
                            symbolEntry.second.symbolName,
                            static_cast<uint>(symbolEntry.second.symbolKind));
        }
    }

    void fillTemporaryLocationsTable(const SourceLocationEntries &sourceLocations)
    {
        WriteStatement &statement = m_insertLocationsToNewLocationsStatement;

        for (const auto &locationEntry : sourceLocations) {
            statement.write(locationEntry.symbolId,
                            locationEntry.lineColumn.line,
                            locationEntry.lineColumn.column,
                            locationEntry.filePathId.filePathId,
                            int(locationEntry.kind));
        }
    }

    void addNewSymbolsToSymbols()
    {
        m_addNewSymbolsToSymbolsStatement.execute();
    }

    void syncNewSymbolsFromSymbols()
    {
        m_syncNewSymbolsFromSymbolsStatement.execute();
    }

    void syncSymbolsIntoNewLocations()
    {
        m_syncSymbolsIntoNewLocationsStatement.execute();
    }

    void deleteAllLocationsFromUpdatedFiles()
    {
        m_deleteAllLocationsFromUpdatedFilesStatement.execute();
    }

    void insertNewLocationsInLocations()
    {
        m_insertNewLocationsInLocationsStatement.execute();
    }

    void deleteNewSymbolsTable()
    {
        m_deleteNewSymbolsTableStatement.execute();
    }

    void deleteNewLocationsTable()
    {
        m_deleteNewLocationsTableStatement.execute();
    }

    Utils::optional<ProjectPartPch> fetchPrecompiledHeader(int projectPartId) const
    {
        return m_getPrecompiledHeader.template value<ProjectPartPch, 2>(projectPartId);
    }

    SourceLocationEntries sourceLocations() const
    {
        return SourceLocationEntries();
    }

    Sqlite::Table createNewSymbolsTable() const
    {
        Sqlite::Table table;
        table.setName("newSymbols");
        table.setUseTemporaryTable(true);
        table.addColumn("temporarySymbolId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        const Sqlite::Column &symbolIdColumn = table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &usrColumn = table.addColumn("usr", Sqlite::ColumnType::Text);
        const Sqlite::Column &symbolNameColumn = table.addColumn("symbolName", Sqlite::ColumnType::Text);
        table.addColumn("symbolKind", Sqlite::ColumnType::Integer);
        table.addIndex({usrColumn, symbolNameColumn});
        table.addIndex({symbolIdColumn});

        table.initialize(m_database);

        return table;
    }

    Sqlite::Table createNewLocationsTable() const
    {
        Sqlite::Table table;
        table.setName("newLocations");
        table.setUseTemporaryTable(true);
        table.addColumn("temporarySymbolId", Sqlite::ColumnType::Integer);
        table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &lineColumn = table.addColumn("line", Sqlite::ColumnType::Integer);
        const Sqlite::Column &columnColumn = table.addColumn("column", Sqlite::ColumnType::Integer);
        table.addColumn("locationKind", Sqlite::ColumnType::Integer);
        table.addUniqueIndex({sourceIdColumn, lineColumn, columnColumn});

        table.initialize(m_database);

        return table;
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction m_transaction;
    Database &m_database;
    Sqlite::Table newSymbolsTablet{createNewSymbolsTable()};
    Sqlite::Table newLocationsTable{createNewLocationsTable()};
    WriteStatement m_insertSymbolsToNewSymbolsStatement{
        "INSERT INTO newSymbols(temporarySymbolId, usr, symbolName, symbolKind) VALUES(?,?,?,?)",
        m_database};
    WriteStatement m_insertLocationsToNewLocationsStatement{
        "INSERT OR IGNORE INTO newLocations(temporarySymbolId, line, column, sourceId, locationKind) VALUES(?,?,?,?,?)",
        m_database
    };
    ReadStatement m_selectNewSourceIdsStatement{
        "SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources WHERE newLocations.sourceId == sources.sourceId)",
        m_database
    };
    WriteStatement m_addNewSymbolsToSymbolsStatement{
        "INSERT INTO symbols(usr, symbolName, symbolKind) "
        "SELECT usr, symbolName, symbolKind FROM newSymbols WHERE NOT EXISTS "
        "(SELECT usr FROM symbols WHERE symbols.usr == newSymbols.usr)",
        m_database
    };
    WriteStatement m_syncNewSymbolsFromSymbolsStatement{
        "UPDATE newSymbols SET symbolId = (SELECT symbolId FROM symbols WHERE newSymbols.usr = symbols.usr)",
        m_database
    };
    WriteStatement m_syncSymbolsIntoNewLocationsStatement{
        "UPDATE newLocations SET symbolId = (SELECT symbolId FROM newSymbols WHERE newSymbols.temporarySymbolId = newLocations.temporarySymbolId)",
        m_database
    };
    WriteStatement m_deleteAllLocationsFromUpdatedFilesStatement{
        "DELETE FROM locations WHERE sourceId IN (SELECT DISTINCT sourceId FROM newLocations)",
        m_database
    };
    WriteStatement m_insertNewLocationsInLocationsStatement{
        "INSERT INTO locations(symbolId, line, column, sourceId, locationKind) SELECT symbolId, line, column, sourceId, locationKind FROM newLocations",
        m_database
    };
    WriteStatement m_deleteNewSymbolsTableStatement{
        "DELETE FROM newSymbols",
        m_database
    };
    WriteStatement m_deleteNewLocationsTableStatement{
        "DELETE FROM newLocations",
        m_database
    };
    WriteStatement m_insertProjectPartStatement{
        "INSERT OR IGNORE INTO projectParts(projectPartName, compilerArguments, compilerMacros, includeSearchPaths) VALUES (?,?,?,?)",
        m_database
    };
    WriteStatement m_updateProjectPartStatement{
        "UPDATE projectParts SET compilerArguments = ?, compilerMacros = ?, includeSearchPaths = ? WHERE projectPartName = ?",
        m_database
    };
    mutable ReadStatement m_getProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?",
        m_database
    };
    WriteStatement m_deleteAllProjectPartsSourcesWithProjectPartIdStatement{
        "DELETE FROM projectPartsSources WHERE projectPartId = ?",
        m_database
    };
    WriteStatement m_insertProjectPartSourcesStatement{
        "INSERT INTO projectPartsSources(projectPartId, sourceId) VALUES (?,?)",
        m_database
    };
    mutable ReadStatement m_getCompileArgumentsForFileIdStatement{
        "SELECT compilerArguments FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)",
        m_database
    };
    mutable ReadStatement m_getProjectPartArtefactsBySourceId{
        "SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)",
        m_database
    };
    mutable ReadStatement m_getProjectPartArtefactsByProjectPartName{
        "SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartName = ?",
        m_database
    };
    mutable ReadStatement m_getPrecompiledHeader{
        "SELECT pchPath, pchBuildTime FROM precompiledHeaders WHERE projectPartId = ?",
        m_database
    };
};

} // namespace ClangBackEnd
