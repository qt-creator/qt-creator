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

template<typename DatabaseType>
class StorageSqliteStatementFactory
{
public:
    using Database = DatabaseType;
    using ReadStatement = typename Database::ReadStatement;
    using WriteStatement = typename Database::WriteStatement;

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

    Sqlite::Table createNewUsedMacrosTable() const
    {
        Sqlite::Table table;
        table.setName("newUsedMacros");
        table.setUseTemporaryTable(true);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &macroNameColumn = table.addColumn("macroName", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, macroNameColumn});

        table.initialize(database);

        return table;
    }

    Sqlite::Table createNewSourceDependenciesTable() const
    {
        Sqlite::Table table;
        table.setName("newSourceDependencies");
        table.setUseTemporaryTable(true);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &dependencySourceIdColumn = table.addColumn("dependencySourceId", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, dependencySourceIdColumn});

        table.initialize(database);

        return table;
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction;
    Database &database;
    Sqlite::Table newSymbolsTablet{createNewSymbolsTable()};
    Sqlite::Table newLocationsTable{createNewLocationsTable()};
    Sqlite::Table newUsedMacroTable{createNewUsedMacrosTable()};
    Sqlite::Table newNewSourceDependenciesTable{createNewSourceDependenciesTable()};
    WriteStatement insertSymbolsToNewSymbolsStatement{
        "INSERT INTO newSymbols(temporarySymbolId, usr, symbolName, symbolKind) VALUES(?,?,?,?)",
        database};
    WriteStatement insertLocationsToNewLocationsStatement{
        "INSERT OR IGNORE INTO newLocations(temporarySymbolId, line, column, sourceId, locationKind) VALUES(?,?,?,?,?)",
        database
    };
    ReadStatement selectNewSourceIdsStatement{
        "SELECT DISTINCT sourceId FROM newLocations WHERE NOT EXISTS (SELECT sourceId FROM sources WHERE newLocations.sourceId == sources.sourceId)",
        database
    };
    WriteStatement addNewSymbolsToSymbolsStatement{
        "INSERT INTO symbols(usr, symbolName, symbolKind) "
        "SELECT usr, symbolName, symbolKind FROM newSymbols WHERE NOT EXISTS "
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
        "INSERT INTO locations(symbolId, line, column, sourceId, locationKind) SELECT symbolId, line, column, sourceId, locationKind FROM newLocations",
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
        "INSERT OR IGNORE INTO projectParts(projectPartName, compilerArguments, compilerMacros, includeSearchPaths) VALUES (?,?,?,?)",
        database
    };
    WriteStatement updateProjectPartStatement{
        "UPDATE projectParts SET compilerArguments = ?, compilerMacros = ?, includeSearchPaths = ? WHERE projectPartName = ?",
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
   WriteStatement insertIntoNewUsedMacrosStatement{
       "INSERT INTO newUsedMacros(sourceId, macroName) VALUES (?,?)",
       database
   };
   WriteStatement syncNewUsedMacrosStatement{
        "INSERT INTO usedMacros(sourceId, macroName) SELECT sourceId, macroName FROM newUsedMacros WHERE NOT EXISTS (SELECT sourceId FROM usedMacros WHERE usedMacros.sourceId == newUsedMacros.sourceId AND usedMacros.macroName == newUsedMacros.macroName)",
        database
    };
   WriteStatement deleteOutdatedUsedMacrosStatement{
        "DELETE FROM usedMacros WHERE sourceId IN (SELECT sourceId FROM newUsedMacros) AND NOT EXISTS (SELECT sourceId FROM newUsedMacros WHERE newUsedMacros.sourceId == usedMacros.sourceId AND newUsedMacros.macroName == usedMacros.macroName)",
        database
    };
   WriteStatement deleteNewUsedMacrosTableStatement{
        "DELETE FROM newUsedMacros",
        database
    };
   WriteStatement insertFileStatuses{
        "INSERT OR REPLACE INTO fileStatuses(sourceId, size, lastModified, isInPrecompiledHeader) VALUES (?,?,?,?)",
        database
    };
   WriteStatement insertIntoNewSourceDependenciesStatement{
       "INSERT INTO newSourceDependencies(sourceId, dependencySourceId) VALUES (?,?)",
       database
   };
   WriteStatement syncNewSourceDependenciesStatement{
        "INSERT INTO sourceDependencies(sourceId, dependencySourceId) SELECT sourceId, dependencySourceId FROM newSourceDependencies WHERE NOT EXISTS (SELECT sourceId FROM sourceDependencies WHERE sourceDependencies.sourceId == newSourceDependencies.sourceId AND sourceDependencies.dependencySourceId == newSourceDependencies.dependencySourceId)",
        database
    };
   WriteStatement deleteOutdatedSourceDependenciesStatement{
       "DELETE FROM sourceDependencies WHERE sourceId IN (SELECT sourceId FROM newSourceDependencies) AND NOT EXISTS (SELECT sourceId FROM newSourceDependencies WHERE newSourceDependencies.sourceId == sourceDependencies.sourceId AND newSourceDependencies.dependencySourceId == sourceDependencies.dependencySourceId)",
       database
   };
   WriteStatement deleteNewSourceDependenciesStatement{
       "DELETE FROM newSourceDependencies",
       database
   };
   ReadStatement getProjectPartArtefactsBySourceId{
       "SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartId = (SELECT projectPartId FROM projectPartsSources WHERE sourceId = ?)",
       database
   };
   ReadStatement getProjectPartArtefactsByProjectPartName{
       "SELECT compilerArguments, compilerMacros, includeSearchPaths, projectPartId FROM projectParts WHERE projectPartName = ?",
       database
   };
   ReadStatement getLowestLastModifiedTimeOfDependencies{
       "WITH RECURSIVE sourceIds(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, sourceIds WHERE sourceDependencies.sourceId = sourceIds.sourceId) SELECT min(lastModified) FROM fileStatuses, sourceIds WHERE fileStatuses.sourceId = sourceIds.sourceId",
       database
   };
   ReadStatement getPrecompiledHeader{
       "SELECT pchPath, pchBuildTime FROM precompiledHeaders WHERE projectPartId = ?",
       database
   };
};
} // namespace ClangBackEnd
