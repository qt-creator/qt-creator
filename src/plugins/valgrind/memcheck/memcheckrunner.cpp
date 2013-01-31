/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "memcheckrunner.h"

#include "../xmlprotocol/error.h"
#include "../xmlprotocol/status.h"
#include "../xmlprotocol/threadedparser.h"

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
          logSocket(0)
    {
    }

    QTcpServer xmlServer;
    XmlProtocol::ThreadedParser *parser;
    QTcpServer logServer;
    QTcpSocket *logSocket;
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
    if (startMode() == Analyzer::StartLocal) {
        QTC_ASSERT(d->parser, return false);

        bool check = d->xmlServer.listen(QHostAddress(QHostAddress::LocalHost));
        QTC_ASSERT(check, return false);
        d->xmlServer.setMaxPendingConnections(1);
        const quint16 xmlPortNumber = d->xmlServer.serverPort();
        connect(&d->xmlServer, SIGNAL(newConnection()), SLOT(xmlSocketConnected()));

        check = d->logServer.listen(QHostAddress(QHostAddress::LocalHost));
        QTC_ASSERT(check, return false);
        d->logServer.setMaxPendingConnections(1);
        const quint16 logPortNumber = d->logServer.serverPort();
        connect(&d->logServer, SIGNAL(newConnection()), SLOT(logSocketConnected()));

        QStringList memcheckArguments;
        memcheckArguments << QLatin1String("--xml=yes")
                          << QString::fromLatin1("--xml-socket=127.0.0.1:%1").arg(xmlPortNumber)
                          << QLatin1String("--child-silent-after-fork=yes")
                          << QString::fromLatin1("--log-socket=127.0.0.1:%1").arg(logPortNumber)
                          << valgrindArguments();
        setValgrindArguments(memcheckArguments);
    }


    if (startMode() == Analyzer::StartRemote) {
        QTC_ASSERT(d->parser, return false);

        QString ip = connectionParameters().host;
        QTC_ASSERT(!ip.isEmpty(), return false);

        QHostAddress hostAddr(ip);
        bool check = d->xmlServer.listen(hostAddr);
        QTC_ASSERT(check, return false);
        d->xmlServer.setMaxPendingConnections(1);
        const quint16 xmlPortNumber = d->xmlServer.serverPort();
        connect(&d->xmlServer, SIGNAL(newConnection()), SLOT(xmlSocketConnected()));

        check = d->logServer.listen(hostAddr);
        QTC_ASSERT(check, return false);
        d->logServer.setMaxPendingConnections(1);
        const quint16 logPortNumber = d->logServer.serverPort();
        connect(&d->logServer, SIGNAL(newConnection()), SLOT(logSocketConnected()));

        QStringList memcheckArguments;
        memcheckArguments << QLatin1String("--xml=yes")
                          << QString::fromLatin1("--xml-socket=%1:%2").arg(ip).arg(xmlPortNumber)
                          << QLatin1String("--child-silent-after-fork=yes")
                          << QString::fromLatin1("--log-socket=%1:%2").arg(ip).arg(logPortNumber);
        setValgrindArguments(memcheckArguments);
    }

    return ValgrindRunner::start();
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
    connect(d->logSocket, SIGNAL(readyRead()),
            this, SLOT(readLogSocket()));
    d->logServer.close();
}

void MemcheckRunner::readLogSocket()
{
    emit logMessageReceived(d->logSocket->readAll());
}

} // namespace Memcheck
} // namespace Valgrind
