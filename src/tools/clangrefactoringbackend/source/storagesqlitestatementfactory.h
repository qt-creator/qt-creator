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
class StorageSqliteStatementFactory
{
public:
    using DatabaseType = Database;
    using ReadStatementType = ReadStatement;
    using WriteStatementType = WriteStatement;

    StorageSqliteStatementFactory(Database &database)
        : transaction(database),
          database(database)
    {
        transaction.commit();
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
        table.addColumn("line", Sqlite::ColumnType::Integer);
        table.addColumn("column", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({sourceIdColumn});

        table.initialize(database);

        return table;
    }

    Sqlite::Table createNewUsedDefinesTable() const
    {
        Sqlite::Table table;
        table.setName("newUsedDefines");
        table.setUseTemporaryTable(true);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &defineNameColumn = table.addColumn("defineName", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, defineNameColumn});

        table.initialize(database);

        return table;
    }

public:
    Sqlite::ImmediateTransaction transaction;
    Database &database;
    Sqlite::Table newSymbolsTablet{createNewSymbolsTable()};
    Sqlite::Table newLocationsTable{createNewLocationsTable()};
    Sqlite::Table newUsedDefineTable{createNewUsedDefinesTable()};
    WriteStatement insertSymbolsToNewSymbolsStatement{
        "INSERT INTO newSymbols(temporarySymbolId, usr, symbolName) VALUES(?,?,?)",
        database};
    WriteStatement insertLocationsToNewLocationsStatement{
        "INSERT INTO newLocations(temporarySymbolId, line, column, sourceId) VALUES(?,?,?,?)",
        database
    };
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
    WriteStatement insertProjectPartStatement{
        "INSERT OR IGNORE INTO projectParts(projectPartName, compilerArguments) VALUES (?,?)",
        database
    };
    WriteStatement updateProjectPartStatement{
        "UPDATE projectParts SET compilerArguments = ? WHERE projectPartName = ?",
        database
    };
    ReadStatement getProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?",
        database
    };
    WriteStatement deleteAllProjectPartsSourcesWithProjectPartIdStatement{
        "DELETE FROM projectPartsSources WHERE projectPartId = ?",
        database
    };
    WriteStatement insertProjectPartSourcesStatement{
        "INSERT INTO projectPartsSources(projectPartId, sourceId) VALUES (?,?)",
        database
    };
   ReadStatement getCompileArgumentsForFileIdStatement{
        "SELECT compilerArguments FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)",
        database
    };
   WriteStatement insertIntoNewUsedDefinesStatement{
       "INSERT INTO newUsedDefines(sourceId, defineName) VALUES (?,?)",
       database
   };
   WriteStatement syncNewUsedDefinesStatement{
        "INSERT INTO usedDefines(sourceId, defineName) SELECT sourceId, defineName FROM newUsedDefines WHERE NOT EXISTS (SELECT sourceId FROM usedDefines WHERE usedDefines.sourceId == newUsedDefines.sourceId AND usedDefines.defineName == newUsedDefines.defineName)",
        database
    };
   WriteStatement deleteOutdatedUsedDefinesStatement{
        "DELETE FROM usedDefines WHERE sourceId IN (SELECT sourceId FROM newUsedDefines) AND NOT EXISTS (SELECT sourceId FROM newUsedDefines WHERE newUsedDefines.sourceId == usedDefines.sourceId AND newUsedDefines.defineName == usedDefines.defineName)",
        database
    };
   WriteStatement deleteNewUsedDefinesTableStatement{
        "DELETE FROM newUsedDefines",
        database
    };
};

} // namespace ClangBackEnd
