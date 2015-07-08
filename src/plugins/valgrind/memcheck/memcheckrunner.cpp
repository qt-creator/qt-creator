/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "memcheckrunner.h"

#include "../xmlprotocol/error.h"
#include "../xmlprotocol/status.h"
#include "../xmlprotocol/threadedparser.h"
#include "../valgrindprocess.h"

#include <utils/qtcassert.h>

#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace Valgrind {
namespace Memcheck {

class MemcheckRunner::Private
{
public:
    explicit Private()
        : parser(0),
          logSocket(0),
          disableXml(false)
    {
    }

    QTcpServer xmlServer;
    XmlProtocol::ThreadedParser *parser;
    QTcpServer logServer;
    QTcpSocket *logSocket;
    bool disableXml;
};

MemcheckRunner::MemcheckRunner(QObject *parent)
    : ValgrindRunner(parent),
      d(new Private)
{
}

MemcheckRunner::~MemcheckRunner()
{
    if (d->parser->isRunning()) {
        // make sure we don't delete the thread while it's still running
        waitForFinished();
    }
    delete d;
    d = 0;
}

QString MemcheckRunner::tool() const
{
    return QLatin1String("memcheck");
}

void MemcheckRunner::setParser(XmlProtocol::ThreadedParser *parser)
{
    QTC_ASSERT(!d->parser, qt_noop());
    d->parser = parser;
}

bool MemcheckRunner::start()
{
    QTC_ASSERT(d->parser, return false);

    // The remote case is handled in localHostAddressRetrieved().
    // FIXME: We start confusing "use startup project" with a "local/remote"
    // decision here.
    if (useStartupProject()) {
        startServers(QHostAddress(QHostAddress::LocalHost));
        setValgrindArguments(memcheckLogArguments() + valgrindArguments());
    }
    return ValgrindRunner::start();
}

// Workaround for valgrind bug when running vgdb with xml output
// https://bugs.kde.org/show_bug.cgi?id=343902
void MemcheckRunner::disableXml()
{
    d->disableXml = true;
}

void MemcheckRunner::xmlSocketConnected()
{
    QTcpSocket *socket = d->xmlServer.nextPendingConnection();
    QTC_ASSERT(socket, return);
    d->xmlServer.close();
    d->parser->parse(socket);
}

void MemcheckRunner::logSocketConnected()
{
    d->logSocket = d->logServer.nextPendingConnection();
    QTC_ASSERT(d->logSocket, return);
    connect(d->logSocket, &QIODevice::readyRead,
            this, &MemcheckRunner::readLogSocket);
    d->logServer.close();
}

void MemcheckRunner::readLogSocket()
{
    QTC_ASSERT(d->logSocket, return);
    emit logMessageReceived(d->logSocket->readAll());
}

// Note: The callers of this function cannot handle errors, so they will ignore the return value.
// We still provide it in case the surrounding infrastructure will improve.
bool MemcheckRunner::startServers(const QHostAddress &localHostAddress)
{
    bool check = d->xmlServer.listen(localHostAddress);
    const QString ip = localHostAddress.toString();
    if (!check) {
        emit processErrorReceived( tr("XmlServer on %1:").arg(ip) + QLatin1Char(' ')
                                   + d->xmlServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    d->xmlServer.setMaxPendingConnections(1);
    connect(&d->xmlServer, &QTcpServer::newConnection,
            this, &MemcheckRunner::xmlSocketConnected);
    check = d->logServer.listen(localHostAddress);
    if (!check) {
        emit processErrorReceived( tr("LogServer on %1:").arg(ip) + QLatin1Char(' ')
                                   + d->logServer.errorString(), QProcess::FailedToStart );
        return false;
    }
    d->logServer.setMaxPendingConnections(1);
    connect(&d->logServer, &QTcpServer::newConnection,
            this, &MemcheckRunner::logSocketConnected);
    return true;
}

QStringList MemcheckRunner::memcheckLogArguments() const
{
    QStringList arguments;
    if (!d->disableXml)
        arguments << QLatin1String("--xml=yes");
    arguments << QString::fromLatin1("--xml-socket=%1:%2")
                 .arg(d->xmlServer.serverAddress().toString()).arg(d->xmlServer.serverPort())
              << QLatin1String("--child-silent-after-fork=yes")
              << QString::fromLatin1("--log-socket=%1:%2")
                 .arg(d->logServer.serverAddress().toString()).arg(d->logServer.serverPort());
    return arguments;
}

void MemcheckRunner::localHostAddressRetrieved(const QHostAddress &localHostAddress)
{
    startServers(localHostAddress);
    setValgrindArguments(memcheckLogArguments() + valgrindArguments());
    valgrindProcess()->setValgrindArguments(fullValgrindArguments());
}

} // namespace Memcheck
} // namespace Valgrind
