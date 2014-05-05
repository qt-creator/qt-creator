/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "qmldebugclient.h"

#include "qpacketprotocol.h"

#include <qdebug.h>
#include <qstringlist.h>
#include <qnetworkproxy.h>

namespace QmlDebug {

const int protocolVersion = 1;
int QmlDebugClient::s_dataStreamVersion = QDataStream::Qt_4_7;

const QString serverId = QLatin1String("QDeclarativeDebugServer");
const QString clientId = QLatin1String("QDeclarativeDebugClient");

class QmlDebugClientPrivate
{
    //    Q_DECLARE_PUBLIC(QmlDebugClient)
public:
    QmlDebugClientPrivate();

    QString name;
    QmlDebugConnection *connection;
};

class QmlDebugConnectionPrivate : public QObject
{
    Q_OBJECT
public:
    QmlDebugConnectionPrivate(QmlDebugConnection *c);
    QmlDebugConnection *q;
    QPacketProtocol *protocol;
    QIODevice *device; // Currently a QTcpSocket

    bool gotHello;
    QHash <QString, float> serverPlugins;
    QHash<QString, QmlDebugClient *> plugins;

    void advertisePlugins();
    void flush();

public Q_SLOTS:
    void connected();
    void readyRead();
};

QmlDebugConnectionPrivate::QmlDebugConnectionPrivate(QmlDebugConnection *c)
    : QObject(c), q(c), protocol(0), device(0), gotHello(false)
{
    QObject::connect(c, SIGNAL(connected()), this, SLOT(connected()));
}

void QmlDebugConnectionPrivate::advertisePlugins()
{
    if (!q->isOpen())
        return;

    QPacket pack;
    pack << serverId << 1 << plugins.keys();
    protocol->send(pack);
    flush();
}

void QmlDebugConnectionPrivate::connected()
{
    QPacket pack;
    QDataStream str;
    pack << serverId << 0 << protocolVersion << plugins.keys() << QDataStream().version();
    protocol->send(pack);
    flush();
}

void QmlDebugConnectionPrivate::readyRead()
{
    if (!gotHello) {
        QPacket pack = protocol->read();
        QString name;

        pack >> name;

        bool validHello = false;
        if (name == clientId) {
            int op = -1;
            pack >> op;
            if (op == 0) {
                int version = -1;
                pack >> version;
                if (version == protocolVersion) {
                    QStringList pluginNames;
                    QList<float> pluginVersions;
                    pack >> pluginNames;
                    if (!pack.isEmpty())
                        pack >> pluginVersions;

                    const int pluginNamesSize = pluginNames.size();
                    const int pluginVersionsSize = pluginVersions.size();
                    for (int i = 0; i < pluginNamesSize; ++i) {
                        float pluginVersion = 1.0;
                        if (i < pluginVersionsSize)
                            pluginVersion = pluginVersions.at(i);
                        serverPlugins.insert(pluginNames.at(i), pluginVersion);
                    }

                    if (!pack.atEnd()) {
                        pack >> QmlDebugClient::s_dataStreamVersion;
                        if (QmlDebugClient::s_dataStreamVersion
                                > QDataStream().version())
                            qWarning() << "Server returned invalid data stream version!";
                    }
                    validHello = true;
                }
            }
        }

        if (!validHello) {
            qWarning("QML Debug Client: Invalid hello message");
            QObject::disconnect(protocol, SIGNAL(readyRead()), this, SLOT(readyRead()));
            return;
        }
        gotHello = true;

        QHash<QString, QmlDebugClient *>::Iterator iter = plugins.begin();
        for (; iter != plugins.end(); ++iter) {
            QmlDebugClient::State newState = QmlDebugClient::Unavailable;
            if (serverPlugins.contains(iter.key()))
                newState = QmlDebugClient::Enabled;
            iter.value()->stateChanged(newState);
        }
    }

    while (protocol->packetsAvailable()) {
        QPacket pack = protocol->read();
        QString name;
        pack >> name;

        if (name == clientId) {
            int op = -1;
            pack >> op;

            if (op == 1) {
                // Service Discovery
                QHash<QString, float> oldServerPlugins = serverPlugins;
                serverPlugins.clear();

                QStringList pluginNames;
                QList<float> pluginVersions;
                pack >> pluginNames;
                if (!pack.isEmpty())
                    pack >> pluginVersions;

                const int pluginNamesSize = pluginNames.size();
                const int pluginVersionsSize = pluginVersions.size();
                for (int i = 0; i < pluginNamesSize; ++i) {
                    float pluginVersion = 1.0;
                    if (i < pluginVersionsSize)
                        pluginVersion = pluginVersions.at(i);
                    serverPlugins.insert(pluginNames.at(i), pluginVersion);
                }

                QHash<QString, QmlDebugClient *>::Iterator iter = plugins.begin();
                for (; iter != plugins.end(); ++iter) {
                    const QString pluginName = iter.key();
                    QmlDebugClient::State newState = QmlDebugClient::Unavailable;
                    if (serverPlugins.contains(pluginName))
                        newState = QmlDebugClient::Enabled;

                    if (oldServerPlugins.contains(pluginName)
                            != serverPlugins.contains(pluginName)) {
                        iter.value()->stateChanged(newState);
                    }
                }
            } else {
                qWarning() << "QML Debug Client: Unknown control message id" << op;
            }
        } else {
            QByteArray message;
            pack >> message;

            QHash<QString, QmlDebugClient *>::Iterator iter =
                    plugins.find(name);
            if (iter == plugins.end())
                qWarning() << "QML Debug Client: Message received for missing plugin" << name;
            else
                (*iter)->messageReceived(message);
        }
    }
}

QmlDebugConnection::QmlDebugConnection(QObject *parent)
    : QObject(parent), d(new QmlDebugConnectionPrivate(this))
{
}

QmlDebugConnection::~QmlDebugConnection()
{
    QHash<QString, QmlDebugClient*>::iterator iter = d->plugins.begin();
    for (; iter != d->plugins.end(); ++iter) {
        iter.value()->d_func()->connection = 0;
        iter.value()->stateChanged(QmlDebugClient::NotConnected);
    }
}

bool QmlDebugConnection::isOpen() const
{
    return socketState() == QAbstractSocket::ConnectedState && d->gotHello;
}

void QmlDebugConnection::close()
{
    if (d->device->isOpen()) {
        d->device->close();
        emit socketStateChanged(QAbstractSocket::UnconnectedState);

        QHash<QString, QmlDebugClient*>::iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter) {
            iter.value()->stateChanged(QmlDebugClient::NotConnected);
        }
    }
}

QString QmlDebugConnection::errorString() const
{
    return d->device->errorString();
}

// For ease of refactoring we use QAbstractSocket's states even if we're actually using a OstChannel underneath
// since serial ports have a subset of the socket states afaics
QAbstractSocket::SocketState QmlDebugConnection::socketState() const
{
    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->device);
    if (socket)
        return socket->state();

    return QAbstractSocket::UnconnectedState;
}

void QmlDebugConnectionPrivate::flush()
{
    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(device);
    if (socket) {
        socket->flush();
        return;
    }
}

void QmlDebugConnection::connectToHost(const QString &hostName, quint16 port)
{
    QTcpSocket *socket = new QTcpSocket(d);
    socket->setProxy(QNetworkProxy::NoProxy);
    d->device = socket;
    d->protocol = new QPacketProtocol(d->device, this);
    connect(d->protocol, SIGNAL(readyRead()), d, SLOT(readyRead()));
    d->gotHello = false;
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SIGNAL(error(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
    socket->connectToHost(hostName, port);
}

//

QmlDebugClientPrivate::QmlDebugClientPrivate()
    : connection(0)
{
}

QmlDebugClient::QmlDebugClient(const QString &name,
                                                 QmlDebugConnection *parent)
    : QObject(parent), d_ptr(new QmlDebugClientPrivate())
{
    Q_D(QmlDebugClient);
    d->name = name;
    d->connection = parent;

    if (!d->connection)
        return;

    if (d->connection->d->plugins.contains(name)) {
        qWarning() << "QML Debug Client: Conflicting plugin name" << name;
        d->connection = 0;
    } else {
        d->connection->d->plugins.insert(name, this);
        d->connection->d->advertisePlugins();
    }
}

QmlDebugClient::~QmlDebugClient()
{
    Q_D(const QmlDebugClient);
    if (d->connection && d->connection->d) {
        d->connection->d->plugins.remove(d->name);
        d->connection->d->advertisePlugins();
    }
}

QString QmlDebugClient::name() const
{
    Q_D(const QmlDebugClient);
    return d->name;
}

float QmlDebugClient::serviceVersion() const
{
    Q_D(const QmlDebugClient);
    if (d->connection && d->connection->d->serverPlugins.contains(d->name))
        return d->connection->d->serverPlugins.value(d->name);
    return -1;
}

QmlDebugClient::State QmlDebugClient::state() const
{
    Q_D(const QmlDebugClient);
    if (!d->connection || !d->connection->isOpen())
        return NotConnected;

    if (d->connection->d->serverPlugins.contains(d->name))
        return Enabled;

    return Unavailable;
}

void QmlDebugClient::sendMessage(const QByteArray &message)
{
    Q_D(QmlDebugClient);
    if (state() != Enabled)
        return;

    QPacket pack;
    pack << d->name << message;
    d->connection->d->protocol->send(pack);
    d->connection->d->flush();
}

void QmlDebugClient::stateChanged(State)
{
}

void QmlDebugClient::messageReceived(const QByteArray &)
{
}

QmlDebugStream::QmlDebugStream()
    : QDataStream()
{
    setVersion(QmlDebugClient::s_dataStreamVersion);
}

QmlDebugStream::QmlDebugStream(QIODevice *d)
    : QDataStream(d)
{
    setVersion(QmlDebugClient::s_dataStreamVersion);
}

QmlDebugStream::QmlDebugStream(QByteArray *ba, QIODevice::OpenMode flags)
    : QDataStream(ba, flags)
{
    setVersion(QmlDebugClient::s_dataStreamVersion);
}

QmlDebugStream::QmlDebugStream(const QByteArray &ba)
    : QDataStream(ba)
{
    setVersion(QmlDebugClient::s_dataStreamVersion);
}

} // namespace QmlDebug

#include <qmldebugclient.moc>
