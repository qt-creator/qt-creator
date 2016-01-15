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

#include "sqlitedatabase.h"

#include "sqlitetable.h"

SqliteDatabase::SqliteDatabase()
    : readDatabaseConnection(QStringLiteral("ReadWorker")),
      writeDatabaseConnection(QStringLiteral("WriterWorker")),
      journalMode_(JournalMode::Wal)
{
    connect(&readDatabaseConnection, &SqliteDatabaseConnectionProxy::connectionIsOpened, this, &SqliteDatabase::handleReadDatabaseConnectionIsOpened);
    connect(&writeDatabaseConnection, &SqliteDatabaseConnectionProxy::connectionIsOpened, this, &SqliteDatabase::handleWriteDatabaseConnectionIsOpened);
    connect(&readDatabaseConnection, &SqliteDatabaseConnectionProxy::connectionIsClosed, this, &SqliteDatabase::handleReadDatabaseConnectionIsClosed);
    connect(&writeDatabaseConnection, &SqliteDatabaseConnectionProxy::connectionIsClosed, this, &SqliteDatabase::handleWriteDatabaseConnectionIsClosed);
}

SqliteDatabase::~SqliteDatabase()
{
    qDeleteAll(sqliteTables);
}

void SqliteDatabase::open()
{
    writeDatabaseConnection.setDatabaseFilePath(databaseFilePath());
    writeDatabaseConnection.setJournalMode(journalMode());
}

void SqliteDatabase::close()
{
    writeDatabaseConnection.close();
}

bool SqliteDatabase::isOpen() const
{
    return readDatabaseConnection.isOpen() && writeDatabaseConnection.isOpen();
}

void SqliteDatabase::addTable(SqliteTable *newSqliteTable)
{
    newSqliteTable->setSqliteDatabase(this);
    sqliteTables.append(newSqliteTable);
}

const QVector<SqliteTable *> &SqliteDatabase::tables() const
{
    return sqliteTables;
}

void SqliteDatabase::setDatabaseFilePath(const QString &databaseFilePath)
{
    databaseFilePath_ = databaseFilePath;
}

const QString &SqliteDatabase::databaseFilePath() const
{
    return databaseFilePath_;
}

void SqliteDatabase::setJournalMode(JournalMode journalMode)
{
    journalMode_ = journalMode;
}

JournalMode SqliteDatabase::journalMode() const
{
    return journalMode_;
}

QThread *SqliteDatabase::writeWorkerThread() const
{
    return writeDatabaseConnection.connectionThread();
}

QThread *SqliteDatabase::readWorkerThread() const
{
    return readDatabaseConnection.connectionThread();
}

void SqliteDatabase::handleReadDatabaseConnectionIsOpened()
{
    if (writeDatabaseConnection.isOpen() && readDatabaseConnection.isOpen()) {
        initializeTables();
        emit databaseIsOpened();
    }
}

void SqliteDatabase::handleWriteDatabaseConnectionIsOpened()
{
    readDatabaseConnection.setDatabaseFilePath(databaseFilePath());
}

void SqliteDatabase::handleReadDatabaseConnectionIsClosed()
{
    if (!writeDatabaseConnection.isOpen() && !readDatabaseConnection.isOpen()) {
        shutdownTables();
        emit databaseIsClosed();
    }
}

void SqliteDatabase::handleWriteDatabaseConnectionIsClosed()
{
    readDatabaseConnection.close();
}

void SqliteDatabase::initializeTables()
{
    for (SqliteTable *table: tables())
        table->initialize();
}

void SqliteDatabase::shutdownTables()
{
    for (SqliteTable *table: tables())
        table->shutdown();
}


