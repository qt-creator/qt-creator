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

#include "sqlitedatabaseconnectionproxy.h"

#include "sqlitedatabaseconnection.h"
#include "sqliteworkerthread.h"

#include <QCoreApplication>


SqliteDatabaseConnectionProxy::SqliteDatabaseConnectionProxy(const QString &threadName) :
    QObject(),
    databaseConnectionIsOpen(false)
{

    databaseConnectionThread = new SqliteWorkerThread;
    databaseConnectionThread->setObjectName(threadName);

    databaseConnectionThread->start(QThread::LowPriority);

    SqliteDatabaseConnection *connection = databaseConnectionThread->databaseConnection();


    connect(this, &SqliteDatabaseConnectionProxy::setDatabaseFilePath, connection, &SqliteDatabaseConnection::setDatabaseFilePath);
    connect(this, &SqliteDatabaseConnectionProxy::setJournalMode, connection, &SqliteDatabaseConnection::setJournalMode);
    connect(this, &SqliteDatabaseConnectionProxy::close, connection, &SqliteDatabaseConnection::close);

    connect(connection, &SqliteDatabaseConnection::databaseConnectionIsOpened, this, &SqliteDatabaseConnectionProxy::handleDatabaseConnectionIsOpened);
    connect(connection, &SqliteDatabaseConnection::databaseConnectionIsClosed, this, &SqliteDatabaseConnectionProxy::handleDatabaseConnectionIsClosed);

}

SqliteDatabaseConnectionProxy::~SqliteDatabaseConnectionProxy()
{
    if (databaseConnectionThread) {
        databaseConnectionThread->quit();
        databaseConnectionThread->wait();
        databaseConnectionThread->deleteLater();
    }
}

QThread *SqliteDatabaseConnectionProxy::connectionThread() const
{
    return databaseConnectionThread;
}

bool SqliteDatabaseConnectionProxy::isOpen() const
{
    return databaseConnectionIsOpen;
}

void SqliteDatabaseConnectionProxy::registerTypes()
{
    qRegisterMetaType<JournalMode>("JournalMode");
}

void SqliteDatabaseConnectionProxy::handleDatabaseConnectionIsOpened()
{
    databaseConnectionIsOpen = true;

    emit connectionIsOpened();
}

void SqliteDatabaseConnectionProxy::handleDatabaseConnectionIsClosed()
{
    databaseConnectionIsOpen = false;

    emit connectionIsClosed();
}
