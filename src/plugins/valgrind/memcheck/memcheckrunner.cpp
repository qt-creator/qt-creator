/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
    QTC_ASSERT(d->parser, return false);

    QString ip;
    QHostAddress hostAddr;

    if (startMode() == Analyzer::StartLocal) {
        ip = QLatin1String("127.0.0.1");
        hostAddr = QHostAddress(QHostAddress::LocalHost);
    }

    if (startMode() == Analyzer::StartRemote) {

        QList<QHostAddress> possibleHostAddresses;
        //NOTE: ::allAddresses does not seem to work for usb interfaces...
        foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces()) {
            foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
                const QHostAddress addr = entry.ip();
                if (addr.toString() != QLatin1String("127.0.0.1")
                        && addr.toString() != QLatin1String("0:0:0:0:0:0:0:1"))
                {
                    possibleHostAddresses << addr;
                    break;
                }
            }
        }

        if (possibleHostAddresses.isEmpty()) {
            emit processErrorReceived(tr("No network interface found for remote analysis."),
                                      QProcess::FailedToStart);
            return false;
        } else if (possibleHostAddresses.size() > 1) {
            QDialog dlg;
            dlg.setWindowTitle(tr("Select Network Interface"));
            QVBoxLayout *layout = new QVBoxLayout;
            QLabel *description = new QLabel;
            description->setWordWrap(true);
            description->setText(tr("More than one network interface was found on your machine. Please select the one you want to use for remote analysis."));
            layout->addWidget(description);
            QListWidget *list = new QListWidget;
            foreach (const QHostAddress &address, possibleHostAddresses)
                list->addItem(address.toString());

            list->setSelectionMode(QAbstractItemView::SingleSelection);
            list->setCurrentRow(0);
            layout->addWidget(list);

            QDialogButtonBox *buttons = new QDialogButtonBox;
            buttons->addButton(QDialogButtonBox::Ok);
            buttons->addButton(QDialogButtonBox::Cancel);
            connect(buttons, SIGNAL(accepted()),
                    &dlg, SLOT(accept()));
            connect(buttons, SIGNAL(rejected()),
                    &dlg, SLOT(reject()));
            layout->addWidget(buttons);

            dlg.setLayout(layout);
            if (dlg.exec() != QDialog::Accepted) {
                emit processErrorReceived(tr("No network interface was chosen for remote analysis."), QProcess::FailedToStart);
                return false;
            }

            QTC_ASSERT(list->currentRow() >= 0, return false);
            QTC_ASSERT(list->currentRow() < possibleHostAddresses.size(), return false);
            hostAddr = possibleHostAddresses.at(list->currentRow());
        } else {
            hostAddr = possibleHostAddresses.first();
        }

        ip = hostAddr.toString();
        QTC_ASSERT(!ip.isEmpty(), return false);
        hostAddr = QHostAddress(ip);
    }

    bool check = d->xmlServer.listen(hostAddr);
    if (!check) emit processErrorReceived( tr("XmlServer on %1:").arg(ip) + QLatin1Char(' ') + d->xmlServer.errorString(), QProcess::FailedToStart );
    QTC_ASSERT(check, return false);
    d->xmlServer.setMaxPendingConnections(1);
    const quint16 xmlPortNumber = d->xmlServer.serverPort();
    connect(&d->xmlServer, SIGNAL(newConnection()), SLOT(xmlSocketConnected()));

    check = d->logServer.listen(hostAddr);
    if (!check) emit processErrorReceived( tr("LogServer on %1:").arg(ip) + QLatin1Char(' ') + d->logServer.errorString(), QProcess::FailedToStart );
    QTC_ASSERT(check, return false);
    d->logServer.setMaxPendingConnections(1);
    const quint16 logPortNumber = d->logServer.serverPort();
    connect(&d->logServer, SIGNAL(newConnection()), SLOT(logSocketConnected()));

    QStringList memcheckLogArguments;
    memcheckLogArguments << QLatin1String("--xml=yes")
                      << QString::fromLatin1("--xml-socket=%1:%2").arg(ip).arg(xmlPortNumber)
                      << QLatin1String("--child-silent-after-fork=yes")
                      << QString::fromLatin1("--log-socket=%1:%2").arg(ip).arg(logPortNumber);
    setValgrindArguments(memcheckLogArguments + valgrindArguments());

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
    QTC_ASSERT(d->logSocket, return);
    emit logMessageReceived(d->logSocket->readAll());
}

} // namespace Memcheck
} // namespace Valgrind
