/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
