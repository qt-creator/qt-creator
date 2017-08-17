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

#include <createtablesqlstatementbuilder.h>

#include <sqlitetransaction.h>
#include <sqlitetable.h>

namespace ClangBackEnd {

template<typename Database,
         typename ReadStatement,
         typename WriteStatement>
class StorageSqliteStatementFactory
{
public:
    using DatabaseType = Database;
    using ReadStatementType = ReadStatement;
    using WriteStatementType = WriteStatement;

    StorageSqliteStatementFactory(Database &database)
        : database(database)
    {
    }

    Sqlite::SqliteTable createSymbolsTable()
    {
        Sqlite::SqliteTable table;
        table.setUseIfNotExists(true);
        table.setName("symbols");
        table.addColumn("symbolId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        const Sqlite::SqliteColumn &usrColumn = table.addColumn("usr", Sqlite::ColumnType::Text);
        table.addColumn("symbolName", Sqlite::ColumnType::Text);
        table.addIndex({usrColumn});

        Sqlite::SqliteImmediateTransaction<DatabaseType> transaction(database);
        table.initialize(database);
        transaction.commit();

        return table;
    }

    Sqlite::SqliteTable createLocationsTable()
    {
        Sqlite::SqliteTable table;
        table.setUseIfNotExists(true);
        table.setName("locations");
        table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        table.addColumn("line", Sqlite::ColumnType::Integer);
        table.addColumn("column", Sqlite::ColumnType::Integer);
        const Sqlite::SqliteColumn &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({sourceIdColumn});

        Sqlite::SqliteImmediateTransaction<DatabaseType> transaction(database);
        table.initialize(database);
        transaction.commit();

        return table;
    }

    Sqlite::SqliteTable createSourcesTable()
    {
        Sqlite::SqliteTable table;
        table.setUseIfNotExists(true);
        table.setName("sources");
        table.addColumn("sourceId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        table.addColumn("sourcePath", Sqlite::ColumnType::Text);

        Sqlite::SqliteImmediateTransaction<DatabaseType> transaction(database);
        table.initialize(database);
        transaction.commit();

        return table;
    }

    Sqlite::SqliteTable createNewSymbolsTable() const
    {
        Sqlite::SqliteTable table;
        table.setName("newSymbols");
        table.setUseTemporaryTable(true);
        table.addColumn("temporarySymbolId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        const Sqlite::SqliteColumn &symbolIdColumn = table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        const Sqlite::SqliteColumn &usrColumn = table.addColumn("usr", Sqlite::ColumnType::Text);
        const Sqlite::SqliteColumn &symbolNameColumn = table.addColumn("symbolName", Sqlite::ColumnType::Text);
        table.addIndex({usrColumn, symbolNameColumn});
        table.addIndex({symbolIdColumn});

        Sqlite::SqliteImmediateTransaction<DatabaseType> transaction(database);
        table.initialize(database);
        transaction.commit();

        return table;
    }

    Sqlite::SqliteTable createNewLocationsTable() const
    {
        Sqlite::SqliteTable table;
        table.setName("newLocations");
        table.setUseTemporaryTable(true);
        table.addColumn("temporarySymbolId", Sqlite::ColumnType::Integer);
        table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        table.addColumn("line", Sqlite::ColumnType::Integer);
        table.addColumn("column", Sqlite::ColumnType::Integer);
        const Sqlite::SqliteColumn &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({sourceIdColumn});

        Sqlite::SqliteImmediateTransaction<DatabaseType> transaction(database);
        table.initialize(database);
        transaction.commit();

        return table;
    }

public:
    Database &database;
    Sqlite::SqliteTable symbolsTable{createSymbolsTable()};
    Sqlite::SqliteTable locationsTable{createLocationsTable()};
    Sqlite::SqliteTable sourcesTable{createSourcesTable()};
    Sqlite::SqliteTable newSymbolsTablet{createNewSymbolsTable()};
    Sqlite::SqliteTable newLocationsTable{createNewLocationsTable()};
    WriteStatement insertSymbolsToNewSymbolsStatement{
        "INSERT INTO newSymbols(temporarySymbolId, usr, symbolName) VALUES(?,?,?)",
        database};
    WriteStatement insertLocationsToNewLocationsStatement{
        "INSERT INTO newLocations(temporarySymbolId, line, column, sourceId) VALUES(?,?,?,?)",
        database
    };
//    WriteStatement syncNewLocationsToLocationsStatement{
//        "INSERT INTO locations(symbolId, line, column, sourceId) SELECT symbolId, line, column, sourceId FROM newLocations",
//        database};
    ReadStatement selectNewSourceIdsStatement{
        "SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources WHERE newLocations.sourceId == sources.sourceId)",
        database
    };
    WriteStatement addNewSymbolsToSymbolsStatement{
        "INSERT INTO symbols(usr, symbolName) "
        "SELECT usr, symbolName FROM newSymbols WHERE NOT EXISTS "
        "(SELECT usr FROM symbols WHERE symbols.usr == newSymbols.usr)",
        database
    };
    WriteStatement insertSourcesStatement{
        "INSERT INTO sources(sourceId, sourcePath) VALUES(?,?)",
        database
    };
    WriteStatement syncNewSymbolsFromSymbolsStatement{
        "UPDATE newSymbols SET symbolId = (SELECT symbolId FROM symbols WHERE newSymbols.usr = symbols.usr)",
        database
    };
    WriteStatement syncSymbolsIntoNewLocationsStatement{
        "UPDATE newLocations SET symbolId = (SELECT symbolId FROM newSymbols WHERE newSymbols.temporarySymbolId = newLocations.temporarySymbolId)",
        database
    };
    WriteStatement deleteAllLocationsFromUpdatedFilesStatement{
        "DELETE FROM locations WHERE sourceId IN (SELECT DISTINCT sourceId FROM newLocations)",
        database
    };
    WriteStatement insertNewLocationsInLocationsStatement{
        "INSERT INTO locations(symbolId, line, column, sourceId) SELECT symbolId, line, column, sourceId FROM newLocations",
        database
    };
    WriteStatement deleteNewSymbolsTableStatement{
        "DELETE FROM newSymbols",
        database
    };
    WriteStatement deleteNewLocationsTableStatement{
        "DELETE FROM newLocations",
        database
    };
};

} // namespace ClangBackEnd
