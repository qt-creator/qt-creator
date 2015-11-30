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

#include "qmldebugclient.h"

#include "qpacketprotocol.h"

#include <qdebug.h>
#include <qstringlist.h>
#include <qnetworkproxy.h>
#include <qlocalserver.h>
#include <qlocalsocket.h>

namespace QmlDebug {

const int protocolVersion = 1;

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

class QmlDebugConnectionPrivate
{
public:
    QmlDebugConnectionPrivate();
    QPacketProtocol *protocol;
    QLocalServer *server;
    QIODevice *device; // Currently a QTcpSocket

    bool gotHello;
    QHash <QString, float> serverPlugins;
    QHash<QString, QmlDebugClient *> plugins;

    int currentDataStreamVersion;
    int maximumDataStreamVersion;

    void advertisePlugins();
    void flush();
};

QString QmlDebugConnection::socketStateToString(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        return tr("Network connection dropped");
    case QAbstractSocket::HostLookupState:
        return tr("Resolving host");
    case QAbstractSocket::ConnectingState:
        return tr("Establishing network connection ...");
    case QAbstractSocket::ConnectedState:
        return tr("Network connection established");
    case QAbstractSocket::ClosingState:
        return tr("Network connection closing");
    case QAbstractSocket::BoundState:
        return tr("Socket state changed to BoundState. This should not happen!");
    case QAbstractSocket::ListeningState:
        return tr("Socket state changed to ListeningState. This should not happen!");
    default:
        return tr("Unknown state %1").arg(state);
    }
}

QString QmlDebugConnection::socketErrorToString(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::RemoteHostClosedError) {
        return tr("Error: Remote host closed the connection");
    } else {
        return tr("Error: Unknown socket error %1").arg(error);
    }
}

QmlDebugConnectionPrivate::QmlDebugConnectionPrivate() :
    protocol(0), server(0), device(0), gotHello(false),
    currentDataStreamVersion(QDataStream::Qt_4_7),
    maximumDataStreamVersion(QDataStream::Qt_DefaultCompiledVersion)
{
}

void QmlDebugConnectionPrivate::advertisePlugins()
{
    if (!gotHello)
        return;

    QPacket pack(currentDataStreamVersion);
    pack << serverId << 1 << plugins.keys();
    protocol->send(pack.data());
    flush();
}

void QmlDebugConnection::socketConnected()
{
    Q_D(QmlDebugConnection);
    QPacket pack(d->currentDataStreamVersion);
    pack << serverId << 0 << protocolVersion << d->plugins.keys() << d->maximumDataStreamVersion;
    d->protocol->send(pack.data());
    d->flush();
}

void QmlDebugConnection::socketDisconnected()
{
    Q_D(QmlDebugConnection);
    if (d->gotHello) {
        d->gotHello = false;
        QHash<QString, QmlDebugClient*>::iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter)
            iter.value()->stateChanged(QmlDebugClient::NotConnected);
        emit disconnected();
    }
    delete d->protocol;
    d->protocol = 0;
    if (d->device) {
        // Don't immediately delete it as it may do some cleanup on returning from a signal.
        d->device->deleteLater();
        d->device = 0;
    }
}

void QmlDebugConnection::protocolReadyRead()
{
    Q_D(QmlDebugConnection);
    if (!d->gotHello) {
        QPacket pack(d->currentDataStreamVersion, d->protocol->read());
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
                    if (!pack.atEnd())
                        pack >> pluginVersions;

                    const int pluginNamesSize = pluginNames.size();
                    const int pluginVersionsSize = pluginVersions.size();
                    for (int i = 0; i < pluginNamesSize; ++i) {
                        float pluginVersion = 1.0;
                        if (i < pluginVersionsSize)
                            pluginVersion = pluginVersions.at(i);
                        d->serverPlugins.insert(pluginNames.at(i), pluginVersion);
                    }

                    if (!pack.atEnd()) {
                        pack >> d->currentDataStreamVersion;
                        if (d->currentDataStreamVersion > d->maximumDataStreamVersion)
                            qWarning() << "Server returned invalid data stream version!";
                    }
                    validHello = true;
                }
            }
        }

        if (!validHello) {
            qWarning("QML Debug Client: Invalid hello message");
            QObject::disconnect(d->protocol, &QPacketProtocol::readyRead,
                                this, &QmlDebugConnection::protocolReadyRead);
            return;
        }
        d->gotHello = true;

        QHash<QString, QmlDebugClient *>::Iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter) {
            QmlDebugClient::State newState = QmlDebugClient::Unavailable;
            if (d->serverPlugins.contains(iter.key()))
                newState = QmlDebugClient::Enabled;
            iter.value()->stateChanged(newState);
        }
        emit connected();
    }

    while (d->protocol && d->protocol->packetsAvailable()) {
        QPacket pack(d->currentDataStreamVersion, d->protocol->read());
        QString name;
        pack >> name;

        if (name == clientId) {
            int op = -1;
            pack >> op;

            if (op == 1) {
                // Service Discovery
                QHash<QString, float> oldServerPlugins = d->serverPlugins;
                d->serverPlugins.clear();

                QStringList pluginNames;
                QList<float> pluginVersions;
                pack >> pluginNames;
                if (!pack.atEnd())
                    pack >> pluginVersions;

                const int pluginNamesSize = pluginNames.size();
                const int pluginVersionsSize = pluginVersions.size();
                for (int i = 0; i < pluginNamesSize; ++i) {
                    float pluginVersion = 1.0;
                    if (i < pluginVersionsSize)
                        pluginVersion = pluginVersions.at(i);
                    d->serverPlugins.insert(pluginNames.at(i), pluginVersion);
                }

                QHash<QString, QmlDebugClient *>::Iterator iter = d->plugins.begin();
                for (; iter != d->plugins.end(); ++iter) {
                    const QString pluginName = iter.key();
                    QmlDebugClient::State newState = QmlDebugClient::Unavailable;
                    if (d->serverPlugins.contains(pluginName))
                        newState = QmlDebugClient::Enabled;

                    if (oldServerPlugins.contains(pluginName)
                            != d->serverPlugins.contains(pluginName)) {
                        iter.value()->stateChanged(newState);
                    }
                }
            } else {
                qWarning() << "QML Debug Client: Unknown control message id" << op;
            }
        } else {
            QByteArray message;
            pack >> message;

            QHash<QString, QmlDebugClient *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end())
                qWarning() << "QML Debug Client: Message received for missing plugin" << name;
            else
                (*iter)->messageReceived(message);
        }
    }
}

QmlDebugConnection::QmlDebugConnection(QObject *parent)
    : QObject(parent), d_ptr(new QmlDebugConnectionPrivate)
{
}

QmlDebugConnection::~QmlDebugConnection()
{
    Q_D(QmlDebugConnection);
    socketDisconnected();
    QHash<QString, QmlDebugClient*>::iterator iter = d->plugins.begin();
    for (; iter != d->plugins.end(); ++iter)
        iter.value()->d_func()->connection = 0;
}

bool QmlDebugConnection::isConnected() const
{
    Q_D(const QmlDebugConnection);
    // gotHello can only be set if the connection is open.
    return d->gotHello;
}

bool QmlDebugConnection::isConnecting() const
{
    Q_D(const QmlDebugConnection);
    return !d->gotHello && d->device;
}

void QmlDebugConnection::close()
{
    Q_D(QmlDebugConnection);
    if (d->device && d->device->isOpen())
        d->device->close(); // will trigger disconnected() at some point.
}

QmlDebugClient *QmlDebugConnection::client(const QString &name) const
{
    Q_D(const QmlDebugConnection);
    return d->plugins.value(name, 0);
}

bool QmlDebugConnection::addClient(const QString &name, QmlDebugClient *client)
{
    Q_D(QmlDebugConnection);
    if (d->plugins.contains(name))
        return false;
    d->plugins.insert(name, client);
    d->advertisePlugins();
    return true;
}

bool QmlDebugConnection::removeClient(const QString &name)
{
    Q_D(QmlDebugConnection);
    if (!d->plugins.contains(name))
        return false;
    d->plugins.remove(name);
    d->advertisePlugins();
    return true;
}

float QmlDebugConnection::serviceVersion(const QString &serviceName) const
{
    Q_D(const QmlDebugConnection);
    return d->serverPlugins.value(serviceName, -1);
}

bool QmlDebugConnection::sendMessage(const QString &name, const QByteArray &message)
{
    Q_D(QmlDebugConnection);
    if (!d->gotHello || !d->serverPlugins.contains(name))
        return false;

    QPacket pack(d->currentDataStreamVersion);
    pack << name << message;
    d->protocol->send(pack.data());
    d->flush();
    return true;
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
    Q_D(QmlDebugConnection);
    socketDisconnected();
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);
    d->device = socket;
    d->protocol = new QPacketProtocol(socket, this);
    QObject::connect(d->protocol, &QPacketProtocol::readyRead,
                     this, &QmlDebugConnection::protocolReadyRead);
    connect(socket, &QAbstractSocket::stateChanged, this, &QmlDebugConnection::socketStateChanged);
    connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>
            (&QAbstractSocket::error), this, &QmlDebugConnection::socketError);
    connect(socket, &QAbstractSocket::connected, this, &QmlDebugConnection::socketConnected);
    connect(socket, &QAbstractSocket::disconnected, this, &QmlDebugConnection::socketDisconnected);
    socket->connectToHost(hostName, port);
}

void QmlDebugConnection::startLocalServer(const QString &fileName)
{
    Q_D(QmlDebugConnection);
    if (d->gotHello)
        close();
    if (d->server)
        d->server->deleteLater();
    d->server = new QLocalServer(this);
    // QueuedConnection so that waitForNewConnection() returns true.
    connect(d->server, SIGNAL(newConnection()), this, SLOT(newConnection()), Qt::QueuedConnection);
    d->server->listen(fileName);
}

void QmlDebugConnection::newConnection()
{
    Q_D(QmlDebugConnection);
    delete d->device;
    QLocalSocket *socket = d->server->nextPendingConnection();
    d->server->close();
    d->device = socket;
    delete d->protocol;
    d->protocol = new QPacketProtocol(socket, this);
    QObject::connect(d->protocol, &QPacketProtocol::readyRead,
                     this, &QmlDebugConnection::protocolReadyRead);

    connect(socket, &QLocalSocket::disconnected, this, &QmlDebugConnection::socketDisconnected);

    connect(socket, static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>
            (&QLocalSocket::error), this, [this](QLocalSocket::LocalSocketError error) {
        socketError(static_cast<QAbstractSocket::SocketError>(error));
    });

    connect(socket, &QLocalSocket::stateChanged,
            this, [this](QLocalSocket::LocalSocketState state) {
        socketStateChanged(static_cast<QAbstractSocket::SocketState>(state));
    });

    socketConnected();
}

int QmlDebugConnection::currentDataStreamVersion() const
{
    Q_D(const QmlDebugConnection);
    return d->currentDataStreamVersion;
}

void QmlDebugConnection::setMaximumDataStreamVersion(int maximumVersion)
{
    Q_D(QmlDebugConnection);
    d->maximumDataStreamVersion = maximumVersion;
}

QAbstractSocket::SocketState QmlDebugConnection::socketState() const
{
    Q_D(const QmlDebugConnection);
    // TODO: when merging into master, add clause for local socket
    if (QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(d->device))
        return socket->state();
    else
        return QAbstractSocket::UnconnectedState;
}

//

QmlDebugClientPrivate::QmlDebugClientPrivate()
    : connection(0)
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

    if (d->connection->client(d->name) == this)
        return Enabled;

    return Unavailable;
}

QmlDebugConnection *QmlDebugClient::connection() const
{
    Q_D(const QmlDebugClient);
    return d->connection;
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
