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
    void disconnected();
    void error(QAbstractSocket::SocketError error);
    void readyRead();
    void stateChanged(QAbstractSocket::SocketState state);
};

QmlDebugConnectionPrivate::QmlDebugConnectionPrivate(QmlDebugConnection *c)
    : QObject(c), q(c), protocol(0), device(0), gotHello(false)
{
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

void QmlDebugConnectionPrivate::disconnected()
{
    if (gotHello) {
        gotHello = false;
        QHash<QString, QmlDebugClient*>::iterator iter = plugins.begin();
        for (; iter != plugins.end(); ++iter)
            iter.value()->stateChanged(QmlDebugClient::NotConnected);
        emit q->closed();
    }
    delete protocol;
    protocol = 0;
    if (device) {
        // Don't immediately delete it as it may do some cleanup on returning from a signal.
        device->deleteLater();
        device = 0;
    }
}

void QmlDebugConnectionPrivate::error(QAbstractSocket::SocketError socketError)
{
    //: %1=error code, %2=error message
    emit q->errorMessage(tr("Error: (%1) %2").arg(socketError)
             .arg(device ? device->errorString() : tr("<device is gone>")));
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        emit q->error(QDebugSupport::RemoteClosedConnectionError);
    else
        emit q->error(QDebugSupport::UnknownError);
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
        emit q->opened();
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

void QmlDebugConnectionPrivate::stateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        emit q->stateMessage(tr("Network connection dropped"));
        break;
    case QAbstractSocket::HostLookupState:
        emit q->stateMessage(tr("Resolving host"));
        break;
    case QAbstractSocket::ConnectingState:
        emit q->stateMessage(tr("Establishing network connection ..."));
        break;
    case QAbstractSocket::ConnectedState:
        emit q->stateMessage(tr("Network connection established"));
        break;
    case QAbstractSocket::ClosingState:
        emit q->stateMessage(tr("Network connection closing"));
        break;
    case QAbstractSocket::BoundState:
        emit q->errorMessage(tr("Socket state changed to BoundState. This should not happen!"));
        break;
    case QAbstractSocket::ListeningState:
        emit q->errorMessage(tr("Socket state changed to ListeningState. This should not happen!"));
        break;
    }
}

QmlDebugConnection::QmlDebugConnection(QObject *parent)
    : QObject(parent), d(new QmlDebugConnectionPrivate(this))
{
}

QmlDebugConnection::~QmlDebugConnection()
{
    d->disconnected();
    QHash<QString, QmlDebugClient*>::iterator iter = d->plugins.begin();
    for (; iter != d->plugins.end(); ++iter)
        iter.value()->d_func()->connection = 0;
}

bool QmlDebugConnection::isOpen() const
{
    // gotHello can only be set if the connection is open.
    return d->gotHello;
}

bool QmlDebugConnection::isConnecting() const
{
    return !isOpen() && d->device;
}

void QmlDebugConnection::close()
{
    if (d->device && d->device->isOpen())
        d->device->close(); // will trigger disconnected() at some point.
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
    d->disconnected();
    emit stateMessage(tr("Connecting to debug server at %1:%2 ...")
             .arg(hostName).arg(QString::number(port)));
    QTcpSocket *socket = new QTcpSocket(d);
    socket->setProxy(QNetworkProxy::NoProxy);
    d->device = socket;
    d->protocol = new QPacketProtocol(d->device, this);
    connect(d->protocol, SIGNAL(readyRead()), d, SLOT(readyRead()));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            d, SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            d, SLOT(error(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(connected()), d, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()), d, SLOT(disconnected()));
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

int QmlDebugClient::remoteVersion() const
{
    Q_D(const QmlDebugClient);
    // The version is internally saved as float for compatibility reasons. Exposing that to clients
    // is a bad idea because floats cannot be properly compared. IEEE 754 floats represent integers
    // exactly up to about 2^24, so the cast shouldn't be a problem for any realistic version
    // numbers.
    if (d->connection && d->connection->d->serverPlugins.contains(d->name))
        return static_cast<int>(d->connection->d->serverPlugins.value(d->name));
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
