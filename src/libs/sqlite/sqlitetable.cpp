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

SqliteTable::SqliteTable()
    : withoutRowId(false)
{

}

SqliteTable::~SqliteTable()
{
    qDeleteAll(sqliteColumns);
}

void SqliteTable::setName(const Utf8String &name)
{
    tableName = name;
}

const Utf8String &SqliteTable::name() const
{
    return tableName;
}

void SqliteTable::setUseWithoutRowId(bool useWithoutWorId)
{
    withoutRowId = useWithoutWorId;
}

bool SqliteTable::useWithoutRowId() const
{
    return withoutRowId;
}

void SqliteTable::addColumn(SqliteColumn *newColumn)
{
    sqliteColumns.append(newColumn);
}

const QVector<SqliteColumn *> &SqliteTable::columns() const
{
    return sqliteColumns;
}

void SqliteTable::setSqliteDatabase(SqliteDatabase *database)
{
    sqliteDatabase = database;
    writeWorker.moveWorkerToThread(sqliteDatabase->writeWorkerThread());
}

void SqliteTable::initialize()
{
    writeWorker.connectWithWorker(this);

    writeWorker.createTable(createTableCommand());
}

void SqliteTable::shutdown()
{
    writeWorker.disconnectWithWorker(this);
}

void SqliteTable::handleTableCreated()
{
    emit tableIsReady();
}

Internal::CreateTableCommand SqliteTable::createTableCommand() const
{
    Internal::CreateTableCommand createTableCommand;

    createTableCommand.tableName = tableName;
    createTableCommand.useWithoutRowId = withoutRowId;
    createTableCommand.definitions = createColumnDefintions();

    return createTableCommand;
}

QVector<Internal::ColumnDefinition> SqliteTable::createColumnDefintions() const
{
    QVector<Internal::ColumnDefinition> columnDefintions;

    for (SqliteColumn *sqliteColumn: sqliteColumns)
        columnDefintions.append(sqliteColumn->columnDefintion());

    return columnDefintions;
}
