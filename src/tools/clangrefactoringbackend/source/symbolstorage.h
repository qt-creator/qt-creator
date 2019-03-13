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
#include <includesearchpath.h>

#include <utils/cpplanguage_details.h>

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
        : transaction(database)
        , database(database)
    {
        transaction.commit();
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

    void fillTemporarySymbolsTable(const SymbolEntries &symbolEntries)
    {
        WriteStatement &statement = insertSymbolsToNewSymbolsStatement;

        for (const auto &symbolEntry : symbolEntries) {
            statement.write(symbolEntry.first,
                            symbolEntry.second.usr,
                            symbolEntry.second.symbolName,
                            static_cast<uint>(symbolEntry.second.symbolKind));
        }
    }

    void fillTemporaryLocationsTable(const SourceLocationEntries &sourceLocations)
    {
        WriteStatement &statement = insertLocationsToNewLocationsStatement;

        for (const auto &locationEntry : sourceLocations) {
            statement.write(locationEntry.symbolId,
                            locationEntry.lineColumn.line,
                            locationEntry.lineColumn.column,
                            locationEntry.filePathId.filePathId,
                            int(locationEntry.kind));
        }
    }

    void addNewSymbolsToSymbols() { addNewSymbolsToSymbolsStatement.execute(); }

    void syncNewSymbolsFromSymbols() { syncNewSymbolsFromSymbolsStatement.execute(); }

    void syncSymbolsIntoNewLocations() { syncSymbolsIntoNewLocationsStatement.execute(); }

    void deleteAllLocationsFromUpdatedFiles()
    {
        deleteAllLocationsFromUpdatedFilesStatement.execute();
    }

    void insertNewLocationsInLocations() { insertNewLocationsInLocationsStatement.execute(); }

    void deleteNewSymbolsTable() { deleteNewSymbolsTableStatement.execute(); }

    void deleteNewLocationsTable() { deleteNewLocationsTableStatement.execute(); }

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

        table.initialize(database);

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

        table.initialize(database);

        return table;
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction;
    Database &database;
    Sqlite::Table newSymbolsTablet{createNewSymbolsTable()};
    Sqlite::Table newLocationsTable{createNewLocationsTable()};
    WriteStatement insertSymbolsToNewSymbolsStatement{
        "INSERT INTO newSymbols(temporarySymbolId, usr, symbolName, symbolKind) VALUES(?,?,?,?)",
        database};
    WriteStatement insertLocationsToNewLocationsStatement{
        "INSERT OR IGNORE INTO newLocations(temporarySymbolId, line, column, sourceId, "
        "locationKind) VALUES(?,?,?,?,?)",
        database};
    ReadStatement selectNewSourceIdsStatement{
        "SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources "
        "WHERE newLocations.sourceId == sources.sourceId)",
        database};
    WriteStatement addNewSymbolsToSymbolsStatement{
        "INSERT INTO symbols(usr, symbolName, symbolKind) "
        "SELECT usr, symbolName, symbolKind FROM newSymbols WHERE NOT EXISTS "
        "(SELECT usr FROM symbols WHERE symbols.usr == newSymbols.usr)",
        database};
    WriteStatement syncNewSymbolsFromSymbolsStatement{
        "UPDATE newSymbols SET symbolId = (SELECT symbolId FROM symbols WHERE newSymbols.usr = "
        "symbols.usr)",
        database};
    WriteStatement syncSymbolsIntoNewLocationsStatement{
        "UPDATE newLocations SET symbolId = (SELECT symbolId FROM newSymbols WHERE "
        "newSymbols.temporarySymbolId = newLocations.temporarySymbolId)",
        database};
    WriteStatement deleteAllLocationsFromUpdatedFilesStatement{
        "DELETE FROM locations WHERE sourceId IN (SELECT DISTINCT sourceId FROM newLocations)",
        database};
    WriteStatement insertNewLocationsInLocationsStatement{
        "INSERT INTO locations(symbolId, line, column, sourceId, locationKind) SELECT symbolId, "
        "line, column, sourceId, locationKind FROM newLocations",
        database};
    WriteStatement deleteNewSymbolsTableStatement{"DELETE FROM newSymbols", database};
    WriteStatement deleteNewLocationsTableStatement{"DELETE FROM newLocations", database};
};

} // namespace ClangBackEnd
