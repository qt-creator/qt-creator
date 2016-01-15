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

#include "sqlitedatabaseconnection.h"

#include "sqliteexception.h"
#include "sqliteglobal.h"

#include <sqlite3.h>

#include <QDebug>

#ifdef Q_OS_LINUX
#include <cerrno>
#include <cstring>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

SqliteDatabaseConnection::SqliteDatabaseConnection(QObject *parent) :
    QObject(parent)
{
}

SqliteDatabaseConnection::~SqliteDatabaseConnection()
{

}

sqlite3 *SqliteDatabaseConnection::currentSqliteDatabase()
{
    return SqliteDatabaseBackend::sqliteDatabaseHandle();
}

void SqliteDatabaseConnection::setDatabaseFilePath(const QString &databaseFilePath)
{

    prioritizeThreadDown();

    try {
        databaseBackend.open(databaseFilePath);

        emit databaseConnectionIsOpened();
    } catch (SqliteException &exception) {
        exception.printWarning();
    }
}

void SqliteDatabaseConnection::setJournalMode(JournalMode journalMode)
{
    try {
        databaseBackend.setJournalMode(journalMode);
    } catch (SqliteException &exception) {
        exception.printWarning();
    }
}

void SqliteDatabaseConnection::close()
{
    databaseBackend.closeWithoutException();

    emit databaseConnectionIsClosed();
}

void SqliteDatabaseConnection::prioritizeThreadDown()
{
#ifdef Q_OS_LINUX
    pid_t processId = syscall(SYS_gettid);
    int returnCode = setpriority(PRIO_PROCESS, processId, 10);
    if (returnCode == -1)
        qWarning() << "cannot renice" <<  strerror(errno);
#endif
}
