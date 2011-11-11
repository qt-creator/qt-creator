/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qdeclarativedebugclient.h"

#include "qpacketprotocol.h"

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtNetwork/qnetworkproxy.h>
#include <symbiandevicemanager.h>

namespace QmlJsDebugClient {

const int protocolVersion = 1;
const QString serverId = QLatin1String("QDeclarativeDebugServer");
const QString clientId = QLatin1String("QDeclarativeDebugClient");
static const uchar KQmlOstProtocolId = 0x94;

class QDeclarativeDebugClientPrivate
{
    //    Q_DECLARE_PUBLIC(QDeclarativeDebugClient)
public:
    QDeclarativeDebugClientPrivate();

    QString name;
    QDeclarativeDebugConnection *connection;
};

class QDeclarativeDebugConnectionPrivate : public QObject
{
    Q_OBJECT
public:
    QDeclarativeDebugConnectionPrivate(QDeclarativeDebugConnection *c);
    QDeclarativeDebugConnection *q;
    QPacketProtocol *protocol;
    QIODevice *device; // Currently either a QTcpSocket or a SymbianUtils::OstChannel

    bool gotHello;
    QStringList serverPlugins;
    QHash<QString, QDeclarativeDebugClient *> plugins;

    void advertisePlugins();
    void connectDeviceSignals();

public Q_SLOTS:
    void connected();
    void readyRead();
    void deviceAboutToClose();
};

QDeclarativeDebugConnectionPrivate::QDeclarativeDebugConnectionPrivate(QDeclarativeDebugConnection *c)
    : QObject(c), q(c), protocol(0), device(0), gotHello(false)
{
    protocol = new QPacketProtocol(q, this);
    QObject::connect(c, SIGNAL(connected()), this, SLOT(connected()));
    QObject::connect(protocol, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

void QDeclarativeDebugConnectionPrivate::advertisePlugins()
{
    if (!q->isConnected() || !gotHello)
        return;

    QPacket pack;
    pack << serverId << 1 << plugins.keys();
    protocol->send(pack);
    q->flush();
}

void QDeclarativeDebugConnectionPrivate::connected()
{
    QPacket pack;
    pack << serverId << 0 << protocolVersion << plugins.keys();
    protocol->send(pack);
    q->flush();
}

void QDeclarativeDebugConnectionPrivate::readyRead()
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
                    pack >> serverPlugins;
                    validHello = true;
                }
            }
        }

        if (!validHello) {
            qWarning("QDeclarativeDebugConnection: Invalid hello message");
            QObject::disconnect(protocol, SIGNAL(readyRead()), this, SLOT(readyRead()));
            return;
        }
        gotHello = true;

        QHash<QString, QDeclarativeDebugClient *>::Iterator iter = plugins.begin();
        for (; iter != plugins.end(); ++iter) {
            QDeclarativeDebugClient::Status newStatus = QDeclarativeDebugClient::Unavailable;
            if (serverPlugins.contains(iter.key()))
                newStatus = QDeclarativeDebugClient::Enabled;
            iter.value()->statusChanged(newStatus);
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
                QStringList oldServerPlugins = serverPlugins;
                pack >> serverPlugins;

                QHash<QString, QDeclarativeDebugClient *>::Iterator iter = plugins.begin();
                for (; iter != plugins.end(); ++iter) {
                    const QString pluginName = iter.key();
                    QDeclarativeDebugClient::Status newStatus = QDeclarativeDebugClient::Unavailable;
                    if (serverPlugins.contains(pluginName))
                        newStatus = QDeclarativeDebugClient::Enabled;

                    if (oldServerPlugins.contains(pluginName)
                            != serverPlugins.contains(pluginName)) {
                        iter.value()->statusChanged(newStatus);
                    }
                }
            } else {
                qWarning() << "QDeclarativeDebugConnection: Unknown control message id" << op;
            }
        } else {
            QByteArray message;
            pack >> message;

            QHash<QString, QDeclarativeDebugClient *>::Iterator iter =
                    plugins.find(name);
            if (iter == plugins.end()) {
                qWarning() << "QDeclarativeDebugConnection: Message received for missing plugin" << name;
            } else {
                (*iter)->messageReceived(message);
            }
        }
    }
}

void QDeclarativeDebugConnectionPrivate::deviceAboutToClose()
{
    // This is nasty syntax but we want to emit our own aboutToClose signal (by calling QIODevice::close())
    // without calling the underlying device close fn as that would cause an infinite loop
    q->QIODevice::close();
}

QDeclarativeDebugConnection::QDeclarativeDebugConnection(QObject *parent)
    : QIODevice(parent), d(new QDeclarativeDebugConnectionPrivate(this))
{
}

QDeclarativeDebugConnection::~QDeclarativeDebugConnection()
{
    QHash<QString, QDeclarativeDebugClient*>::iterator iter = d->plugins.begin();
    for (; iter != d->plugins.end(); ++iter) {
        iter.value()->d_func()->connection = 0;
        iter.value()->statusChanged(QDeclarativeDebugClient::NotConnected);
    }
}

bool QDeclarativeDebugConnection::isConnected() const
{
    return state() == QAbstractSocket::ConnectedState;
}

qint64 QDeclarativeDebugConnection::readData(char *data, qint64 maxSize)
{
    return d->device->read(data, maxSize);
}

qint64 QDeclarativeDebugConnection::writeData(const char *data, qint64 maxSize)
{
    return d->device->write(data, maxSize);
}

void QDeclarativeDebugConnection::internalError(QAbstractSocket::SocketError socketError)
{
    setErrorString(d->device->errorString());
    emit error(socketError);
}

qint64 QDeclarativeDebugConnection::bytesAvailable() const
{
    return d->device->bytesAvailable();
}

bool QDeclarativeDebugConnection::isSequential() const
{
    return true;
}

void QDeclarativeDebugConnection::close()
{
    if (isOpen()) {
        QIODevice::close();
        d->device->close();
        emit stateChanged(QAbstractSocket::UnconnectedState);

        QHash<QString, QDeclarativeDebugClient*>::iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter) {
            iter.value()->statusChanged(QDeclarativeDebugClient::NotConnected);
        }
    }
}

bool QDeclarativeDebugConnection::waitForConnected(int msecs)
{
    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->device);
    if (socket)
        return socket->waitForConnected(msecs);
    return false;
}

// For ease of refactoring we use QAbstractSocket's states even if we're actually using a OstChannel underneath
// since serial ports have a subset of the socket states afaics
QAbstractSocket::SocketState QDeclarativeDebugConnection::state() const
{
    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->device);
    if (socket)
        return socket->state();

    SymbianUtils::OstChannel *ost = qobject_cast<SymbianUtils::OstChannel*>(d->device);
    if (ost) {
        //TODO we need some handshaking here
        /*
        if (ost->hasReceivedData())
            return QAbstractSocket::ConnectedState;
        else if (ost->isOpen())
            return QAbstractSocket::ConnectingState;
        */
        if (ost->isOpen()) return QAbstractSocket::ConnectedState;
    }

    return QAbstractSocket::UnconnectedState;
}

void QDeclarativeDebugConnection::flush()
{
    QAbstractSocket *socket = qobject_cast<QAbstractSocket*>(d->device);
    if (socket) {
        socket->flush();
        return;
    }

    SymbianUtils::OstChannel *ost = qobject_cast<SymbianUtils::OstChannel*>(d->device);
    if (ost) {
        ost->flush();
        return;
    }
}

void QDeclarativeDebugConnection::connectToHost(const QString &hostName, quint16 port)
{
    QTcpSocket *socket = new QTcpSocket(d);
    socket->setProxy(QNetworkProxy::NoProxy);
    d->device = socket;
    d->connectDeviceSignals();
    d->gotHello = false;
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(internalError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
    socket->connectToHost(hostName, port);
    QIODevice::open(ReadWrite | Unbuffered);
}

void QDeclarativeDebugConnection::connectToOst(const QString &port)
{
    SymbianUtils::OstChannel *ost = SymbianUtils::SymbianDeviceManager::instance()->getOstChannel(port, KQmlOstProtocolId);
    if (ost) {
        ost->setParent(d);
        d->device = ost;
        d->connectDeviceSignals();
        d->gotHello = false;
        QIODevice::open(ReadWrite | Unbuffered);
        emit stateChanged(QAbstractSocket::ConnectedState);
        emit connected();
    } else {
        emit error(QAbstractSocket::HostNotFoundError);
    }
}

void QDeclarativeDebugConnectionPrivate::connectDeviceSignals()
{
    connect(device, SIGNAL(bytesWritten(qint64)), q, SIGNAL(bytesWritten(qint64)));
    connect(device, SIGNAL(readyRead()), q, SIGNAL(readyRead()));
    connect(device, SIGNAL(aboutToClose()), this, SLOT(deviceAboutToClose()));
}

//

QDeclarativeDebugClientPrivate::QDeclarativeDebugClientPrivate()
    : connection(0)
{
}

QDeclarativeDebugClient::QDeclarativeDebugClient(const QString &name, 
                                                 QDeclarativeDebugConnection *parent)
    : QObject(parent), d_ptr(new QDeclarativeDebugClientPrivate())
{
    Q_D(QDeclarativeDebugClient);
    d->name = name;
    d->connection = parent;

    if (!d->connection)
        return;

    if (d->connection->d->plugins.contains(name)) {
        qWarning() << "QDeclarativeDebugClient: Conflicting plugin name" << name;
        d->connection = 0;
    } else {
        d->connection->d->plugins.insert(name, this);
        d->connection->d->advertisePlugins();
    }
}

QDeclarativeDebugClient::~QDeclarativeDebugClient()
{
    Q_D(const QDeclarativeDebugClient);
    if (d->connection && d->connection->d) {
        d->connection->d->plugins.remove(d->name);
        d->connection->d->advertisePlugins();
    }
}

QString QDeclarativeDebugClient::name() const
{
    Q_D(const QDeclarativeDebugClient);
    return d->name;
}

QDeclarativeDebugClient::Status QDeclarativeDebugClient::status() const
{
    Q_D(const QDeclarativeDebugClient);
    if (!d->connection
            || !d->connection->isConnected()
            || !d->connection->d->gotHello)
        return NotConnected;

    if (d->connection->d->serverPlugins.contains(d->name))
        return Enabled;

    return Unavailable;
}

void QDeclarativeDebugClient::sendMessage(const QByteArray &message)
{
    Q_D(QDeclarativeDebugClient);
    if (status() != Enabled)
        return;

    QPacket pack;
    pack << d->name << message;
    d->connection->d->protocol->send(pack);
    d->connection->flush();
}

void QDeclarativeDebugClient::statusChanged(Status)
{
}

void QDeclarativeDebugClient::messageReceived(const QByteArray &)
{
}

} // namespace QmlJsDebugClient

#include <qdeclarativedebugclient.moc>
