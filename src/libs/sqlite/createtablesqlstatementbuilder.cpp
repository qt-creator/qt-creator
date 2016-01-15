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

namespace Internal {

CreateTableSqlStatementBuilder::CreateTableSqlStatementBuilder()
    : sqlStatementBuilder(Utf8StringLiteral("CREATE TABLE IF NOT EXISTS $table($columnDefinitions)$withoutRowId")),
      useWithoutRowId(false)
{
}

void CreateTableSqlStatementBuilder::setTable(const Utf8String &tableName)
{
    sqlStatementBuilder.clear();

    this->tableName = tableName;
}

void CreateTableSqlStatementBuilder::addColumnDefinition(const Utf8String &columnName,
                                                         ColumnType columnType,
                                                         bool isPrimaryKey)
{
    sqlStatementBuilder.clear();

    ColumnDefinition columnDefinition;
    columnDefinition.setName(columnName);
    columnDefinition.setType(columnType);
    columnDefinition.setIsPrimaryKey(isPrimaryKey);

    columnDefinitions.append(columnDefinition);
}

void CreateTableSqlStatementBuilder::setColumnDefinitions(const QVector<ColumnDefinition> &columnDefinitions)
{
    sqlStatementBuilder.clear();

    this->columnDefinitions = columnDefinitions;
}

void CreateTableSqlStatementBuilder::setUseWithoutRowId(bool useWithoutRowId)
{
    this->useWithoutRowId = useWithoutRowId;
}

void CreateTableSqlStatementBuilder::clear()
{
    sqlStatementBuilder.clear();
    columnDefinitions.clear();
    tableName.clear();
    useWithoutRowId = false;
}

void CreateTableSqlStatementBuilder::clearColumns()
{
    sqlStatementBuilder.clear();
    columnDefinitions.clear();
}

Utf8String CreateTableSqlStatementBuilder::sqlStatement() const
{
    if (!sqlStatementBuilder.isBuild())
        bindAll();

    return sqlStatementBuilder.sqlStatement();
}

bool CreateTableSqlStatementBuilder::isValid() const
{
    return tableName.hasContent() && !columnDefinitions.isEmpty();
}

void CreateTableSqlStatementBuilder::bindColumnDefinitions() const
{
    Utf8StringVector columnDefinitionStrings;

    foreach (const ColumnDefinition &columnDefinition, columnDefinitions) {
        Utf8String columnDefinitionString = columnDefinition.name() + Utf8StringLiteral(" ") + columnDefinition.typeString();

        if (columnDefinition.isPrimaryKey())
            columnDefinitionString.append(Utf8StringLiteral(" PRIMARY KEY"));

        columnDefinitionStrings.append(columnDefinitionString);
    }

    sqlStatementBuilder.bind(Utf8StringLiteral("$columnDefinitions"), columnDefinitionStrings);
}

void CreateTableSqlStatementBuilder::bindAll() const
{
    sqlStatementBuilder.bind(Utf8StringLiteral("$table"), tableName);

    bindColumnDefinitions();

    if (useWithoutRowId)
        sqlStatementBuilder.bind(Utf8StringLiteral("$withoutRowId"), Utf8StringLiteral(" WITHOUT ROWID"));
    else
        sqlStatementBuilder.bindEmptyText(Utf8StringLiteral("$withoutRowId"));
}

}
