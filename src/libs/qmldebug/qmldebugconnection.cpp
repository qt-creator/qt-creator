/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qmldebugconnection.h"
#include "qmldebugclient.h"
#include "qpacketprotocol.h"

#include <utils/porting.h>
#include <utils/temporaryfile.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)

namespace QmlDebug {

const int protocolVersion = 1;

const QString serverId = QLatin1String("QDeclarativeDebugServer");
const QString clientId = QLatin1String("QDeclarativeDebugClient");

class QmlDebugConnectionPrivate
{
public:
    QPacketProtocol *protocol = nullptr;
    QLocalServer *server = nullptr;
    QIODevice *device = nullptr; // Currently a QTcpSocket or a QLocalSocket

    bool gotHello = false;
    QHash <QString, float> serverPlugins;
    QHash<QString, QmlDebugClient *> plugins;

    int currentDataStreamVersion = QmlDebugConnection::minimumDataStreamVersion();
    int maximumDataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;

    void advertisePlugins();
    void flush();
};

static QString socketStateToString(QAbstractSocket::SocketState state)
{
    QString stateString;
    QDebug(&stateString) << state;
    return QmlDebugConnection::tr("Socket state changed to %1").arg(stateString);
}

static QString socketErrorToString(QAbstractSocket::SocketError error)
{
    QString errorString;
    QDebug(&errorString) << error;
    return QmlDebugConnection::tr("Error: %1").arg(errorString);
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
    pack << serverId << 0 << protocolVersion << d->plugins.keys() << d->maximumDataStreamVersion
         << true; // We accept multiple messages per packet
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
    } else if (d->device) {
        emit connectionFailed();
    }
    if (d->protocol) {
        d->protocol->disconnect();
        d->protocol->deleteLater();
        d->protocol = nullptr;
    }
    if (d->device) {
        // Don't allow any "connected()" or "disconnected()" signals to be triggered anymore.
        // As the protocol is gone this would lead to crashes.
        d->device->disconnect();
        // Don't immediately delete it as it may do some cleanup on returning from a signal.
        d->device->deleteLater();
        d->device = nullptr;
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
            close();
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
                    const QString &pluginName = iter.key();
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
            QHash<QString, QmlDebugClient *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end()) {
                qWarning() << "QML Debug Client: Message received for missing plugin" << name;
            } else {
                QmlDebugClient *client = *iter;
                QByteArray message;
                while (!pack.atEnd()) {
                    pack >> message;
                    client->messageReceived(message);
                }
            }
        }
    }
}

QmlDebugConnection::QmlDebugConnection(QObject *parent)
    : QObject(parent), d_ptr(new QmlDebugConnectionPrivate)
{
    static const int metaTypes[] = {
        qRegisterMetaType<QAbstractSocket::SocketError>(),
        qRegisterMetaType<QLocalSocket::LocalSocketError>()
    };
    Q_UNUSED(metaTypes)
}

QmlDebugConnection::~QmlDebugConnection()
{
    socketDisconnected();
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
    return d->plugins.value(name, nullptr);
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
    if (auto socket = qobject_cast<QAbstractSocket *>(device))
        socket->flush();
    else if (auto socket = qobject_cast<QLocalSocket *>(device))
        socket->flush();
}

void QmlDebugConnection::connectToHost(const QString &hostName, quint16 port)
{
    Q_D(QmlDebugConnection);
    socketDisconnected();
    auto socket = new QTcpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);
    d->device = socket;
    d->protocol = new QPacketProtocol(socket, this);
    QObject::connect(d->protocol, &QPacketProtocol::readyRead,
                     this, &QmlDebugConnection::protocolReadyRead);
    connect(socket, &QAbstractSocket::stateChanged,
            this, [this](QAbstractSocket::SocketState state) {
        emit logStateChange(socketStateToString(state));
    });

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const auto errorOccurred = QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error);
#else
    const auto errorOccurred = &QAbstractSocket::errorOccurred;
#endif
    connect(socket, errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        emit logError(socketErrorToString(error));
        socketDisconnected();
    }, Qt::QueuedConnection);
    connect(socket, &QAbstractSocket::connected, this, &QmlDebugConnection::socketConnected);
    connect(socket, &QAbstractSocket::disconnected, this, &QmlDebugConnection::socketDisconnected,
            Qt::QueuedConnection);
    socket->connectToHost(hostName.isEmpty() ? QString("localhost") : hostName, port);
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
    connect(d->server, &QLocalServer::newConnection,
            this, &QmlDebugConnection::newConnection, Qt::QueuedConnection);
    if (!d->server->listen(fileName))
        emit connectionFailed();
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

    connect(socket, &QLocalSocket::disconnected, this, &QmlDebugConnection::socketDisconnected,
            Qt::QueuedConnection);

    constexpr void (QLocalSocket::*LocalSocketErrorFunction)(QLocalSocket::LocalSocketError)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                = &QLocalSocket::error;
#else
                = &QLocalSocket::errorOccurred;
#endif

    connect(socket, LocalSocketErrorFunction, this, [this](QLocalSocket::LocalSocketError error) {
        emit logError(socketErrorToString(static_cast<QAbstractSocket::SocketError>(error)));
        socketDisconnected();
    }, Qt::QueuedConnection);

    connect(socket, &QLocalSocket::stateChanged,
            this, [this](QLocalSocket::LocalSocketState state) {
        emit logStateChange(socketStateToString(static_cast<QAbstractSocket::SocketState>(state)));
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
    if (auto socket = qobject_cast<QAbstractSocket *>(d->device))
        return socket->state();
    if (auto socket = qobject_cast<QLocalSocket *>(d->device))
        return static_cast<QAbstractSocket::SocketState>(socket->state());
    return QAbstractSocket::UnconnectedState;
}

} // namespace QmlDebug
