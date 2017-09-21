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

template<typename DatabaseType>
class RefactoringDatabaseInitializer
{
public:
    RefactoringDatabaseInitializer(DatabaseType &database)
        : database(database)
    {
        Sqlite::ImmediateTransaction<DatabaseType> transaction{database};

        createSymbolsTable();
        createLocationsTable();
        createSourcesTable();
        createDirectoriesTable();

        transaction.commit();
    }

    void createSymbolsTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("symbols");
        table.addColumn("symbolId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        const Sqlite::Column &usrColumn = table.addColumn("usr", Sqlite::ColumnType::Text);
        table.addColumn("symbolName", Sqlite::ColumnType::Text);
        table.addIndex({usrColumn});

        table.initialize(database);
    }

    void createLocationsTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("locations");
        table.addColumn("symbolId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &lineColumn = table.addColumn("line", Sqlite::ColumnType::Integer);
        const Sqlite::Column &columnColumn = table.addColumn("column", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({sourceIdColumn, lineColumn, columnColumn});

        table.initialize(database);
    }

    void createSourcesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("sources");
        table.addColumn("sourceId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        table.addColumn("directoryId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceNameColumn = table.addColumn("sourceName", Sqlite::ColumnType::Text);
        table.addColumn("sourceType", Sqlite::ColumnType::Integer);
        table.addIndex({sourceNameColumn});

        table.initialize(database);
    }

    void createDirectoriesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("directories");
        table.addColumn("directoryId", Sqlite::ColumnType::Integer, Sqlite::Contraint::PrimaryKey);
        const Sqlite::Column &directoryPathColumn = table.addColumn("directoryPath", Sqlite::ColumnType::Text);
        table.addIndex({directoryPathColumn});

        table.initialize(database);
    }

public:
    DatabaseType &database;
};

} // namespace ClangBackEnd
