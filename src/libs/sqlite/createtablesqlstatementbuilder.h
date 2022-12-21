// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlitecolumn.h"
#include "sqlstatementbuilder.h"
#include "tableconstraints.h"

#include <type_traits>

namespace Sqlite {

template<typename ColumnType>
class CreateTableSqlStatementBuilder
{
public:
    CreateTableSqlStatementBuilder()
        : m_sqlStatementBuilder(templateText())
    {}

    void setTableName(Utils::SmallString &&tableName)
    {
        m_sqlStatementBuilder.clear();

        this->m_tableName = std::move(tableName);
    }

    void addColumn(Utils::SmallStringView columnName,
                   ColumnType columnType,
                   Constraints &&constraints = {})
    {
        m_sqlStatementBuilder.clear();

        m_columns.emplace_back(Utils::SmallStringView{}, columnName, columnType, std::move(constraints));
    }
    void addConstraint(TableConstraint &&constraint)
    {
        m_tableConstraints.push_back(std::move(constraint));
    }
    void setConstraints(TableConstraints constraints)
    {
        m_tableConstraints = std::move(constraints);
    }
    void setColumns(BasicColumns<ColumnType> columns)
    {
        m_sqlStatementBuilder.clear();

        m_columns = std::move(columns);
    }

    void setUseWithoutRowId(bool useWithoutRowId) { m_useWithoutRowId = useWithoutRowId; }

    void setUseIfNotExists(bool useIfNotExists) { m_useIfNotExits = useIfNotExists; }

    void setUseTemporaryTable(bool useTemporaryTable) { m_useTemporaryTable = useTemporaryTable; }

    void clear()
    {
        m_sqlStatementBuilder.clear();
        m_columns.clear();
        m_tableName.clear();
        m_useWithoutRowId = false;
    }

    void clearColumns()
    {
        m_sqlStatementBuilder.clear();
        m_columns.clear();
    }

    Utils::SmallStringView sqlStatement() const
    {
        if (!m_sqlStatementBuilder.isBuild())
            bindAll();

        return m_sqlStatementBuilder.sqlStatement();
    }

    bool isValid() const { return m_tableName.hasContent() && !m_columns.empty(); }

private:
    static Utils::SmallStringView templateText()
    {
        if constexpr (std::is_same_v<ColumnType, ::Sqlite::ColumnType>) {
            return "CREATE $temporaryTABLE $ifNotExits$table($columnDefinitions)$withoutRowId";
        }

        return "CREATE $temporaryTABLE $ifNotExits$table($columnDefinitions)$withoutRowId STRICT";
    }

    static Utils::SmallString columnTypeToString(ColumnType columnType)
    {
        if constexpr (std::is_same_v<ColumnType, ::Sqlite::ColumnType>) {
            switch (columnType) {
            case ColumnType::Numeric:
                return " NUMERIC";
            case ColumnType::Integer:
                return " INTEGER";
            case ColumnType::Real:
                return " REAL";
            case ColumnType::Text:
                return " TEXT";
            case ColumnType::Blob:
                return " BLOB";
            case ColumnType::None:
                return {};
            }
        } else {
            switch (columnType) {
            case ColumnType::Any:
                return " ANY";
            case ColumnType::Int:
                return " INT";
            case ColumnType::Integer:
                return " INTEGER";
            case ColumnType::Real:
                return " REAL";
            case ColumnType::Text:
                return " TEXT";
            case ColumnType::Blob:
                return " BLOB";
            }
        }

        return "";
    }

    static Utils::SmallStringView actionToText(ForeignKeyAction action)
    {
        switch (action) {
        case ForeignKeyAction::NoAction:
            return "NO ACTION";
        case ForeignKeyAction::Restrict:
            return "RESTRICT";
        case ForeignKeyAction::SetNull:
            return "SET NULL";
        case ForeignKeyAction::SetDefault:
            return "SET DEFAULT";
        case ForeignKeyAction::Cascade:
            return "CASCADE";
        }

        return "";
    }

    class ContraintsVisiter
    {
    public:
        ContraintsVisiter(Utils::SmallString &columnDefinitionString)
            : columnDefinitionString(columnDefinitionString)
        {}

        void operator()(const Unique &) { columnDefinitionString.append(" UNIQUE"); }

        void operator()(const PrimaryKey &primaryKey)
        {
            columnDefinitionString.append(" PRIMARY KEY");
            if (primaryKey.autoincrement == AutoIncrement::Yes)
                columnDefinitionString.append(" AUTOINCREMENT");
        }

        void operator()(const ForeignKey &foreignKey)
        {
            columnDefinitionString.append(" REFERENCES ");
            columnDefinitionString.append(foreignKey.table);

            if (foreignKey.column.hasContent()) {
                columnDefinitionString.append("(");
                columnDefinitionString.append(foreignKey.column);
                columnDefinitionString.append(")");
            }

            if (foreignKey.updateAction != ForeignKeyAction::NoAction) {
                columnDefinitionString.append(" ON UPDATE ");
                columnDefinitionString.append(actionToText(foreignKey.updateAction));
            }

            if (foreignKey.deleteAction != ForeignKeyAction::NoAction) {
                columnDefinitionString.append(" ON DELETE ");
                columnDefinitionString.append(actionToText(foreignKey.deleteAction));
            }

            if (foreignKey.enforcement == Enforment::Deferred)
                columnDefinitionString.append(" DEFERRABLE INITIALLY DEFERRED");
        }

        void operator()(const NotNull &) { columnDefinitionString.append(" NOT NULL"); }

        void operator()(const Check &check)
        {
            columnDefinitionString.append(" CHECK (");
            columnDefinitionString.append(check.expression);
            columnDefinitionString.append(")");
        }

        void operator()(const DefaultValue &defaultValue)
        {
            columnDefinitionString.append(" DEFAULT ");
            switch (defaultValue.value.type()) {
            case Sqlite::ValueType::Integer:
                columnDefinitionString.append(
                    Utils::SmallString::number(defaultValue.value.toInteger()));
                break;
            case Sqlite::ValueType::Float:
                columnDefinitionString.append(Utils::SmallString::number(defaultValue.value.toFloat()));
                break;
            case Sqlite::ValueType::String:
                columnDefinitionString.append("'");
                columnDefinitionString.append(defaultValue.value.toStringView());
                columnDefinitionString.append("'");
                break;
            default:
                break;
            }
        }

        void operator()(const DefaultExpression &defaultexpression)
        {
            columnDefinitionString.append(" DEFAULT (");
            columnDefinitionString.append(defaultexpression.expression);
            columnDefinitionString.append(")");
        }

        void operator()(const Collate &collate)
        {
            columnDefinitionString.append(" COLLATE ");
            columnDefinitionString.append(collate.collation);
        }

        void operator()(const GeneratedAlways &generatedAlways)
        {
            columnDefinitionString.append(" GENERATED ALWAYS AS (");
            columnDefinitionString.append(generatedAlways.expression);
            columnDefinitionString.append(")");

            if (generatedAlways.storage == Sqlite::GeneratedAlwaysStorage::Virtual)
                columnDefinitionString.append(" VIRTUAL");
            else
                columnDefinitionString.append(" STORED");
        }

        Utils::SmallString &columnDefinitionString;
    };

    class TableContraintsVisiter
    {
    public:
        TableContraintsVisiter(Utils::SmallString &columnDefinitionString)
            : columnDefinitionString(columnDefinitionString)
        {}

        void operator()(const TablePrimaryKey &primaryKey)
        {
            columnDefinitionString.append("PRIMARY KEY(");
            columnDefinitionString.append(primaryKey.columns.join(", "));
            columnDefinitionString.append(")");
        }

        Utils::SmallString &columnDefinitionString;
    };

    void bindColumnDefinitionsAndTableConstraints() const
    {
        Utils::SmallStringVector columnDefinitionStrings;
        columnDefinitionStrings.reserve(m_columns.size());

        for (const BasicColumn<ColumnType> &column : m_columns) {
            auto columnDefinitionString = Utils::SmallString::join(
                {column.name, columnTypeToString(column.type)});

            ContraintsVisiter visiter{columnDefinitionString};

            for (const Constraint &constraint : column.constraints)
                std::visit(visiter, constraint);

            columnDefinitionStrings.push_back(std::move(columnDefinitionString));
        }

        for (const TableConstraint &constraint : m_tableConstraints) {
            Utils::SmallString columnDefinitionString;

            TableContraintsVisiter visiter{columnDefinitionString};
            std::visit(visiter, constraint);

            columnDefinitionStrings.push_back(std::move(columnDefinitionString));
        }

        m_sqlStatementBuilder.bind("$columnDefinitions", columnDefinitionStrings);
    }

    void bindAll() const
    {
        m_sqlStatementBuilder.bind("$table", m_tableName.clone());

        bindTemporary();
        bindIfNotExists();
        bindColumnDefinitionsAndTableConstraints();
        bindWithoutRowId();
    }

    void bindWithoutRowId() const
    {
        if (m_useWithoutRowId) {
            if constexpr (std::is_same_v<ColumnType, ::Sqlite::ColumnType>)
                m_sqlStatementBuilder.bind("$withoutRowId", " WITHOUT ROWID");
            else
                m_sqlStatementBuilder.bind("$withoutRowId", " WITHOUT ROWID,");
        } else {
            m_sqlStatementBuilder.bindEmptyText("$withoutRowId");
        }
    }

    void bindIfNotExists() const
    {
        if (m_useIfNotExits)
            m_sqlStatementBuilder.bind("$ifNotExits", "IF NOT EXISTS ");
        else
            m_sqlStatementBuilder.bindEmptyText("$ifNotExits");
    }

    void bindTemporary() const
    {
        if (m_useTemporaryTable)
            m_sqlStatementBuilder.bind("$temporary", "TEMPORARY ");
        else
            m_sqlStatementBuilder.bindEmptyText("$temporary");
    }

private:
    mutable SqlStatementBuilder m_sqlStatementBuilder;
    Utils::SmallString m_tableName;
    BasicColumns<ColumnType> m_columns;
    TableConstraints m_tableConstraints;
    bool m_useWithoutRowId = false;
    bool m_useIfNotExits = false;
    bool m_useTemporaryTable = false;
};

} // namespace Sqlite
