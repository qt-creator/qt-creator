/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "createtablesqlstatementbuilder.h"
#include "sqliteglobal.h"
#include "sqlitecolumn.h"
#include "sqliteindex.h"
#include "sqliteexception.h"

namespace Sqlite {

class Database;

class Table
{
public:
    Table(std::size_t reserve = 10)
    {
        m_sqliteColumns.reserve(reserve);
        m_sqliteIndices.reserve(reserve);
    }

    void setName(Utils::SmallString &&name)
    {
        m_tableName = std::move(name);
    }

    Utils::SmallStringView name() const
    {
        return m_tableName;
    }

    void setUseWithoutRowId(bool useWithoutWorId)
    {
        m_withoutRowId = useWithoutWorId;
    }

    bool useWithoutRowId() const
    {
        return m_withoutRowId;
    }

    void setUseIfNotExists(bool useIfNotExists)
    {
        m_useIfNotExists = useIfNotExists;
    }

    void setUseTemporaryTable(bool useTemporaryTable)
    {
        m_useTemporaryTable = useTemporaryTable;
    }

    Column &addColumn(Utils::SmallString &&name,
                            ColumnType type = ColumnType::Numeric,
                            Contraint constraint = Contraint::NoConstraint)
    {
        m_sqliteColumns.emplace_back(std::move(name), type, constraint);

        return m_sqliteColumns.back();
    }

    Index &addIndex(const SqliteColumnConstReferences &columns)
    {
        m_sqliteIndices.emplace_back(m_tableName.clone(), sqliteColumnNames(columns));

        return m_sqliteIndices.back();
    }

    const SqliteColumns &columns() const
    {
        return m_sqliteColumns;
    }

    bool isReady() const
    {
        return m_isReady;
    }

    template <typename Database>
    void initialize(Database &database)
    {
        CreateTableSqlStatementBuilder builder;

        builder.setTableName(m_tableName.clone());
        builder.setUseWithoutRowId(m_withoutRowId);
        builder.setUseIfNotExists(m_useIfNotExists);
        builder.setUseTemporaryTable(m_useTemporaryTable);
        builder.setColumns(m_sqliteColumns);

        database.execute(builder.sqlStatement());

        initializeIndices(database);

        m_isReady = true;
    }
    template <typename Database>
    void initializeIndices(Database &database)
    {
        for (const Index &index : m_sqliteIndices)
            database.execute(index.sqlStatement());
    }

    friend bool operator==(const Table &first, const Table &second)
    {
        return first.m_tableName == second.m_tableName
            && first.m_withoutRowId == second.m_withoutRowId
            && first.m_useIfNotExists == second.m_useIfNotExists
            && first.m_isReady == second.m_isReady
            && first.m_sqliteColumns == second.m_sqliteColumns;
    }

private:
    Utils::SmallStringVector sqliteColumnNames(const SqliteColumnConstReferences &columns)
    {
        Utils::SmallStringVector columnNames;

        for (const Column &column : columns)
            columnNames.push_back(column.name());

        return columnNames;
    }

private:
    Utils::SmallString m_tableName;
    SqliteColumns m_sqliteColumns;
    SqliteIndices m_sqliteIndices;
    bool m_withoutRowId = false;
    bool m_useIfNotExists = false;
    bool m_useTemporaryTable = false;
    bool m_isReady = false;
};

} // namespace Sqlite
