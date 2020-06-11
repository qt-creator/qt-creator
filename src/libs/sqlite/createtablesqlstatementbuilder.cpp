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

#include "createtablesqlstatementbuilder.h"

namespace Sqlite {

CreateTableSqlStatementBuilder::CreateTableSqlStatementBuilder()
    : m_sqlStatementBuilder("CREATE $temporaryTABLE $ifNotExits$table($columnDefinitions)$withoutRowId")
{
}

void CreateTableSqlStatementBuilder::setTableName(Utils::SmallString &&tableName)
{
    m_sqlStatementBuilder.clear();

    this->m_tableName = std::move(tableName);
}

void CreateTableSqlStatementBuilder::addColumn(Utils::SmallStringView columnName,
                                               ColumnType columnType,
                                               Constraints &&constraints)
{
    m_sqlStatementBuilder.clear();

    m_columns.emplace_back(Utils::SmallStringView{}, columnName, columnType, std::move(constraints));
}

void CreateTableSqlStatementBuilder::addConstraint(TableConstraint &&constraint)
{
    m_tableConstraints.push_back(std::move(constraint));
}

void CreateTableSqlStatementBuilder::setConstraints(TableConstraints constraints)
{
    m_tableConstraints = std::move(constraints);
}

void CreateTableSqlStatementBuilder::setColumns(SqliteColumns columns)
{
    m_sqlStatementBuilder.clear();

    m_columns = std::move(columns);
}

void CreateTableSqlStatementBuilder::setUseWithoutRowId(bool useWithoutRowId)
{
    m_useWithoutRowId = useWithoutRowId;
}

void CreateTableSqlStatementBuilder::setUseIfNotExists(bool useIfNotExists)
{
    m_useIfNotExits = useIfNotExists;
}

void CreateTableSqlStatementBuilder::setUseTemporaryTable(bool useTemporaryTable)
{
    m_useTemporaryTable = useTemporaryTable;
}

void CreateTableSqlStatementBuilder::clear()
{
    m_sqlStatementBuilder.clear();
    m_columns.clear();
    m_tableName.clear();
    m_useWithoutRowId = false;
}

void CreateTableSqlStatementBuilder::clearColumns()
{
    m_sqlStatementBuilder.clear();
    m_columns.clear();
}

Utils::SmallStringView CreateTableSqlStatementBuilder::sqlStatement() const
{
    if (!m_sqlStatementBuilder.isBuild())
        bindAll();

    return m_sqlStatementBuilder.sqlStatement();
}

bool CreateTableSqlStatementBuilder::isValid() const
{
    return m_tableName.hasContent() && !m_columns.empty();
}

namespace {
Utils::SmallStringView actionToText(ForeignKeyAction action)
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
} // namespace
void CreateTableSqlStatementBuilder::bindColumnDefinitionsAndTableConstraints() const
{
    Utils::SmallStringVector columnDefinitionStrings;
    columnDefinitionStrings.reserve(m_columns.size());

    for (const Column &column : m_columns) {
        Utils::SmallString columnDefinitionString = {column.name, " ", column.typeString()};

        ContraintsVisiter visiter{columnDefinitionString};

        for (const Constraint &constraint : column.constraints)
            Utils::visit(visiter, constraint);

        columnDefinitionStrings.push_back(std::move(columnDefinitionString));
    }

    for (const TableConstraint &constraint : m_tableConstraints) {
        Utils::SmallString columnDefinitionString;

        TableContraintsVisiter visiter{columnDefinitionString};
        Utils::visit(visiter, constraint);

        columnDefinitionStrings.push_back(std::move(columnDefinitionString));
    }

    m_sqlStatementBuilder.bind("$columnDefinitions", columnDefinitionStrings);
}

void CreateTableSqlStatementBuilder::bindAll() const
{
    m_sqlStatementBuilder.bind("$table", m_tableName.clone());

    bindTemporary();
    bindIfNotExists();
    bindColumnDefinitionsAndTableConstraints();
    bindWithoutRowId();
}

void CreateTableSqlStatementBuilder::bindWithoutRowId() const
{
    if (m_useWithoutRowId)
        m_sqlStatementBuilder.bind("$withoutRowId", " WITHOUT ROWID");
    else
        m_sqlStatementBuilder.bindEmptyText("$withoutRowId");
}

void CreateTableSqlStatementBuilder::bindIfNotExists() const
{
    if (m_useIfNotExits)
        m_sqlStatementBuilder.bind("$ifNotExits", "IF NOT EXISTS ");
    else
        m_sqlStatementBuilder.bindEmptyText("$ifNotExits");
}

void CreateTableSqlStatementBuilder::bindTemporary() const
{
    if (m_useTemporaryTable)
        m_sqlStatementBuilder.bind("$temporary", "TEMPORARY ");
    else
        m_sqlStatementBuilder.bindEmptyText("$temporary");
}

} // namespace Sqlite
