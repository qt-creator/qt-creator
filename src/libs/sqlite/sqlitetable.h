// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtablesqlstatementbuilder.h"
#include "sqliteglobal.h"
#include "sqlitecolumn.h"
#include "sqliteindex.h"
#include "sqliteexception.h"

namespace Sqlite {

class Database;

template<typename ColumnType>
class BasicTable
{
public:
    using Column = ::Sqlite::BasicColumn<ColumnType>;
    using ColumnConstReferences = ::Sqlite::BasicColumnConstReferences<ColumnType>;
    using Columns = ::Sqlite::BasicColumns<ColumnType>;

    BasicTable(std::size_t reserve = 10)
    {
        m_sqliteColumns.reserve(reserve);
        m_sqliteIndices.reserve(reserve);
    }

    void setName(Utils::SmallStringView name) { m_tableName = name; }

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

    Column &addColumn(Utils::SmallStringView name, ColumnType type = {}, Constraints &&constraints = {})
    {
        m_sqliteColumns.emplace_back(m_tableName, name, type, std::move(constraints));

        return m_sqliteColumns.back();
    }

    Column &addForeignKeyColumn(Utils::SmallStringView name,
                                const BasicTable &referencedTable,
                                ForeignKeyAction foreignKeyupdateAction = {},
                                ForeignKeyAction foreignKeyDeleteAction = {},
                                Enforment foreignKeyEnforcement = {},
                                Constraints &&constraints = {},
                                ColumnType type = ColumnType::Integer)
    {
        constraints.emplace_back(ForeignKey{referencedTable.name(),
                                            "",
                                            foreignKeyupdateAction,
                                            foreignKeyDeleteAction,
                                            foreignKeyEnforcement});

        m_sqliteColumns.emplace_back(m_tableName, name, type, std::move(constraints));

        return m_sqliteColumns.back();
    }

    Column &addForeignKeyColumn(Utils::SmallStringView name,
                                const BasicColumn<ColumnType> &referencedColumn,
                                ForeignKeyAction foreignKeyupdateAction = {},
                                ForeignKeyAction foreignKeyDeleteAction = {},
                                Enforment foreignKeyEnforcement = {},
                                Constraints &&constraints = {})
    {
        if (!constainsUniqueIndex(referencedColumn.constraints))
            throw ForeignKeyColumnIsNotUnique();

        constraints.emplace_back(ForeignKey{referencedColumn.tableName,
                                            referencedColumn.name,
                                            foreignKeyupdateAction,
                                            foreignKeyDeleteAction,
                                            foreignKeyEnforcement});

        m_sqliteColumns.emplace_back(m_tableName,
                                     name,
                                     referencedColumn.type,
                                     std::move(constraints));

        return m_sqliteColumns.back();
    }

    void addPrimaryKeyContraint(const BasicColumnConstReferences<ColumnType> &columns)
    {
        Utils::SmallStringVector columnNames;
        columnNames.reserve(columns.size());

        for (const auto &column : columns)
            columnNames.emplace_back(column.name);

        m_tableConstraints.emplace_back(TablePrimaryKey{std::move(columnNames)});
    }

    Index &addIndex(const BasicColumnConstReferences<ColumnType> &columns,
                    Utils::SmallStringView condition = {})
    {
        return m_sqliteIndices.emplace_back(m_tableName,
                                            sqliteColumnNames(columns),
                                            IndexType::Normal,
                                            condition);
    }

    Index &addUniqueIndex(const BasicColumnConstReferences<ColumnType> &columns,
                          Utils::SmallStringView condition = {})
    {
        return m_sqliteIndices.emplace_back(m_tableName,
                                            sqliteColumnNames(columns),
                                            IndexType::Unique,
                                            condition);
    }

    const Columns &columns() const { return m_sqliteColumns; }

    bool isReady() const
    {
        return m_isReady;
    }

    template <typename Database>
    void initialize(Database &database)
    {
        CreateTableSqlStatementBuilder<ColumnType> builder;

        builder.setTableName(m_tableName.clone());
        builder.setUseWithoutRowId(m_withoutRowId);
        builder.setUseIfNotExists(m_useIfNotExists);
        builder.setUseTemporaryTable(m_useTemporaryTable);
        builder.setColumns(m_sqliteColumns);
        builder.setConstraints(m_tableConstraints);

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

    friend bool operator==(const BasicTable &first, const BasicTable &second)
    {
        return first.m_tableName == second.m_tableName
            && first.m_withoutRowId == second.m_withoutRowId
            && first.m_useIfNotExists == second.m_useIfNotExists
            && first.m_isReady == second.m_isReady
            && first.m_sqliteColumns == second.m_sqliteColumns;
    }

    static bool constainsUniqueIndex(const Constraints &constraints)
    {
        return std::find_if(constraints.begin(),
                            constraints.end(),
                            [](const Constraint &constraint) {
                                return std::holds_alternative<Unique>(constraint)
                                       || std::holds_alternative<PrimaryKey>(constraint);
                            })
               != constraints.end();
    }

private:
    Utils::SmallStringVector sqliteColumnNames(const ColumnConstReferences &columns)
    {
        Utils::SmallStringVector columnNames;

        for (const Column &column : columns)
            columnNames.push_back(column.name);

        return columnNames;
    }

private:
    Utils::SmallString m_tableName;
    Columns m_sqliteColumns;
    SqliteIndices m_sqliteIndices;
    TableConstraints m_tableConstraints;
    bool m_withoutRowId = false;
    bool m_useIfNotExists = false;
    bool m_useTemporaryTable = false;
    bool m_isReady = false;
};

using Table = BasicTable<ColumnType>;
using StrictTable = BasicTable<StrictColumnType>;

} // namespace Sqlite
