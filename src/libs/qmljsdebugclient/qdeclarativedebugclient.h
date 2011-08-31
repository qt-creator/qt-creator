/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QDECLARATIVEDEBUGCLIENT_H
#define QDECLARATIVEDEBUGCLIENT_H

#include "qmljsdebugclient_global.h"
#include <QtNetwork/qtcpsocket.h>

namespace QmlJsDebugClient {

class QDeclarativeDebugConnectionPrivate;
class QMLJSDEBUGCLIENT_EXPORT QDeclarativeDebugConnection : public QIODevice
{
    Q_OBJECT
    Q_DISABLE_COPY(QDeclarativeDebugConnection)
public:
    QDeclarativeDebugConnection(QObject * = 0);
    ~QDeclarativeDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);
    void connectToOst(const QString &port);

    qint64 bytesAvailable() const;
    bool isConnected() const;
    QAbstractSocket::SocketState state() const;
    void flush();
    bool isSequential() const;
    void close();
    bool waitForConnected(int msecs = 30000);

signals:
    void connected();
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QDeclarativeDebugConnectionPrivate *d;
    friend class QDeclarativeDebugClient;
    friend class QDeclarativeDebugClientPrivate;
};

class QDeclarativeDebugClientPrivate;
class QMLJSDEBUGCLIENT_EXPORT QDeclarativeDebugClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeDebugClient)
    Q_DISABLE_COPY(QDeclarativeDebugClient)

public:
    enum Status { NotConnected, Unavailable, Enabled };

    QDeclarativeDebugClient(const QString &, QDeclarativeDebugConnection *parent);
    ~QDeclarativeDebugClient();

    QString name() const;

    Status status() const;

    virtual void sendMessage(const QByteArray &);

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    friend class QDeclarativeDebugConnection;
    friend class QDeclarativeDebugConnectionPrivate;
    QScopedPointer<QDeclarativeDebugClientPrivate> d_ptr;
};

} // namespace QmlJsDebugClient

#endif // QDECLARATIVEDEBUGCLIENT_H
