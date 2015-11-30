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
