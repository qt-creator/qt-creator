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

#include "qmldebugclient.h"
#include "qmldebugconnection.h"
#include "qpacketprotocol.h"

#include <qdebug.h>
#include <qstringlist.h>

#include <QPointer>

namespace QmlDebug {

class QmlDebugClientPrivate
{
public:
    QmlDebugClientPrivate();

    QString name;
    QPointer<QmlDebugConnection> connection;
};

QmlDebugClientPrivate::QmlDebugClientPrivate()
{
}

QmlDebugClient::QmlDebugClient(const QString &name, QmlDebugConnection *parent)
    : QObject(parent), d_ptr(new QmlDebugClientPrivate())
{
    Q_D(QmlDebugClient);
    d->name = name;
    d->connection = parent;

    if (!d->connection)
        return;

    d->connection->addClient(name, this);
}

QmlDebugClient::~QmlDebugClient()
{
    Q_D(const QmlDebugClient);

    if (d->connection)
        d->connection->removeClient(d->name);
}

QString QmlDebugClient::name() const
{
    Q_D(const QmlDebugClient);
    return d->name;
}

float QmlDebugClient::serviceVersion() const
{
    Q_D(const QmlDebugClient);
    // The version is internally saved as float for compatibility reasons. Exposing that to clients
    // is a bad idea because floats cannot be properly compared. IEEE 754 floats represent integers
    // exactly up to about 2^24, so the cast shouldn't be a problem for any realistic version
    // numbers.
    if (d->connection)
        return d->connection->serviceVersion(d->name);
    return -1;
}

QmlDebugClient::State QmlDebugClient::state() const
{
    Q_D(const QmlDebugClient);
    if (!d->connection || !d->connection->isConnected())
        return NotConnected;

    if (d->connection->serviceVersion(d->name) != -1)
        return Enabled;

    return Unavailable;
}

QmlDebugConnection *QmlDebugClient::connection() const
{
    Q_D(const QmlDebugClient);
    return d->connection;
}

int QmlDebugClient::dataStreamVersion() const
{
    Q_D(const QmlDebugClient);
    return (d->connection ? d->connection->currentDataStreamVersion()
                          : QmlDebugConnection::minimumDataStreamVersion());
}

void QmlDebugClient::sendMessage(const QByteArray &message)
{
    Q_D(QmlDebugClient);
    if (state() != Enabled)
        return;

    d->connection->sendMessage(d->name, message);
}

void QmlDebugClient::stateChanged(State)
{
}

void QmlDebugClient::messageReceived(const QByteArray &)
{
}

} // namespace QmlDebug
