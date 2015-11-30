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
