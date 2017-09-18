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

void CreateTableSqlStatementBuilder::addColumn(Utils::SmallString &&columnName,
                                               ColumnType columnType,
                                               Contraint constraint)
{
    m_sqlStatementBuilder.clear();

    m_columns.emplace_back(std::move(columnName), columnType, constraint);
}

void CreateTableSqlStatementBuilder::setColumns(const SqliteColumns &columns)
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

void CreateTableSqlStatementBuilder::bindColumnDefinitions() const
{
    Utils::SmallStringVector columnDefinitionStrings;

    for (const Column &columns : m_columns) {
        Utils::SmallString columnDefinitionString = {columns.name(), " ", columns.typeString()};

        switch (columns.constraint()) {
            case Contraint::PrimaryKey: columnDefinitionString.append(" PRIMARY KEY"); break;
            case Contraint::Unique: columnDefinitionString.append(" UNIQUE"); break;
            case Contraint::NoConstraint: break;
        }

        columnDefinitionStrings.push_back(columnDefinitionString);
    }

    m_sqlStatementBuilder.bind("$columnDefinitions", columnDefinitionStrings);
}

void CreateTableSqlStatementBuilder::bindAll() const
{
    m_sqlStatementBuilder.bind("$table", m_tableName.clone());

    bindTemporary();
    bindIfNotExists();
    bindColumnDefinitions();
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
