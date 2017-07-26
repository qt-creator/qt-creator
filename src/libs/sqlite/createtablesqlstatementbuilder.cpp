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

#include "utf8stringvector.h"

namespace Sqlite {

CreateTableSqlStatementBuilder::CreateTableSqlStatementBuilder()
    : m_sqlStatementBuilder(Utf8StringLiteral("CREATE TABLE IF NOT EXISTS $table($columnDefinitions)$withoutRowId")),
      m_useWithoutRowId(false)
{
}

void CreateTableSqlStatementBuilder::setTable(const Utf8String &tableName)
{
    m_sqlStatementBuilder.clear();

    this->m_tableName = tableName;
}

void CreateTableSqlStatementBuilder::addColumnDefinition(const Utf8String &columnName,
                                                         ColumnType columnType,
                                                         bool isPrimaryKey)
{
    m_sqlStatementBuilder.clear();

    ColumnDefinition columnDefinition;
    columnDefinition.setName(columnName);
    columnDefinition.setType(columnType);
    columnDefinition.setIsPrimaryKey(isPrimaryKey);

    m_columnDefinitions.append(columnDefinition);
}

void CreateTableSqlStatementBuilder::setColumnDefinitions(const QVector<ColumnDefinition> &columnDefinitions)
{
    m_sqlStatementBuilder.clear();

    this->m_columnDefinitions = columnDefinitions;
}

void CreateTableSqlStatementBuilder::setUseWithoutRowId(bool useWithoutRowId)
{
    this->m_useWithoutRowId = useWithoutRowId;
}

void CreateTableSqlStatementBuilder::clear()
{
    m_sqlStatementBuilder.clear();
    m_columnDefinitions.clear();
    m_tableName.clear();
    m_useWithoutRowId = false;
}

void CreateTableSqlStatementBuilder::clearColumns()
{
    m_sqlStatementBuilder.clear();
    m_columnDefinitions.clear();
}

Utf8String CreateTableSqlStatementBuilder::sqlStatement() const
{
    if (!m_sqlStatementBuilder.isBuild())
        bindAll();

    return m_sqlStatementBuilder.sqlStatement();
}

bool CreateTableSqlStatementBuilder::isValid() const
{
    return m_tableName.hasContent() && !m_columnDefinitions.isEmpty();
}

void CreateTableSqlStatementBuilder::bindColumnDefinitions() const
{
    Utf8StringVector columnDefinitionStrings;

    foreach (const ColumnDefinition &columnDefinition, m_columnDefinitions) {
        Utf8String columnDefinitionString = columnDefinition.name() + Utf8StringLiteral(" ") + columnDefinition.typeString();

        if (columnDefinition.isPrimaryKey())
            columnDefinitionString.append(Utf8StringLiteral(" PRIMARY KEY"));

        columnDefinitionStrings.append(columnDefinitionString);
    }

    m_sqlStatementBuilder.bind(Utf8StringLiteral("$columnDefinitions"), columnDefinitionStrings);
}

void CreateTableSqlStatementBuilder::bindAll() const
{
    m_sqlStatementBuilder.bind(Utf8StringLiteral("$table"), m_tableName);

    bindColumnDefinitions();

    if (m_useWithoutRowId)
        m_sqlStatementBuilder.bind(Utf8StringLiteral("$withoutRowId"), Utf8StringLiteral(" WITHOUT ROWID"));
    else
        m_sqlStatementBuilder.bindEmptyText(Utf8StringLiteral("$withoutRowId"));
}

} // namespace Sqlite
