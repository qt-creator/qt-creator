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

#include "sqlitetable.h"

#include "sqlitecolumn.h"
#include "sqlitedatabase.h"
#include "createtablesqlstatementbuilder.h"
#include "sqlitewritestatement.h"
#include "sqlitetransaction.h"

namespace Sqlite {

SqliteTable::SqliteTable()
    : m_withoutRowId(false)
{

}

SqliteTable::~SqliteTable()
{
    qDeleteAll(m_sqliteColumns);
}

void SqliteTable::setName(Utils::SmallString &&name)
{
    m_tableName = std::move(name);
}

Utils::SmallStringView SqliteTable::name() const
{
    return m_tableName;
}

void SqliteTable::setUseWithoutRowId(bool useWithoutWorId)
{
    m_withoutRowId = useWithoutWorId;
}

bool SqliteTable::useWithoutRowId() const
{
    return m_withoutRowId;
}

void SqliteTable::addColumn(SqliteColumn *newColumn)
{
    m_sqliteColumns.push_back(newColumn);
}

const std::vector<SqliteColumn *> &SqliteTable::columns() const
{
    return m_sqliteColumns;
}

void SqliteTable::setSqliteDatabase(SqliteDatabase *database)
{
    m_sqliteDatabase = database;
}

void SqliteTable::initialize()
{
    try {
        CreateTableSqlStatementBuilder createTableSqlStatementBuilder;

        createTableSqlStatementBuilder.setTable(m_tableName.clone());
        createTableSqlStatementBuilder.setUseWithoutRowId(m_withoutRowId);
        createTableSqlStatementBuilder.setColumnDefinitions(createColumnDefintions());

        SqliteImmediateTransaction transaction(*m_sqliteDatabase);
        SqliteWriteStatement(createTableSqlStatementBuilder.sqlStatement(), *m_sqliteDatabase).step();
        transaction.commit();

        m_isReady = true;

    } catch (const SqliteException &exception) {
        exception.printWarning();
    }
}

bool SqliteTable::isReady() const
{
    return m_isReady;
}

ColumnDefinitions SqliteTable::createColumnDefintions() const
{
    ColumnDefinitions columnDefintions;

    for (SqliteColumn *sqliteColumn : m_sqliteColumns)
        columnDefintions.push_back(sqliteColumn->columnDefintion());

    return columnDefintions;
}

} // namespace Sqlite
