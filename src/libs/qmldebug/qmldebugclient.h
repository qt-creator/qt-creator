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

#ifndef QMLDEBUGCLIENT_H
#define QMLDEBUGCLIENT_H

#include "qmldebug_global.h"
#include <qtcpsocket.h>

#include <QDataStream>

namespace QDebugSupport {
enum Error {
    RemoteClosedConnectionError,
    UnknownError
};
}

namespace QmlDebug {

class QmlDebugConnectionPrivate;
class QMLDEBUG_EXPORT QmlDebugConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlDebugConnection)
public:
    QmlDebugConnection(QObject * = 0);
    ~QmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);

    bool isOpen() const;
    bool isConnecting() const;
    void close();

signals:
    void opened();
    void error(QDebugSupport::Error);
    void closed();
    void stateMessage(const QString &message);
    void errorMessage(const QString &message);

private:
    friend class QmlDebugConnectionPrivate;
    QmlDebugConnectionPrivate *d;
    friend class QmlDebugClient;
    friend class QmlDebugClientPrivate;
};

class QmlDebugClientPrivate;
class QMLDEBUG_EXPORT QmlDebugClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QmlDebugClient)
    Q_DISABLE_COPY(QmlDebugClient)

public:
    enum State { NotConnected, Unavailable, Enabled };

    QmlDebugClient(const QString &, QmlDebugConnection *parent);
    ~QmlDebugClient();

    QString name() const;
    int remoteVersion() const;
    State state() const;

    virtual void sendMessage(const QByteArray &);

protected:
    virtual void stateChanged(State);
    virtual void messageReceived(const QByteArray &);

private:
    friend class QmlDebugConnection;
    friend class QmlDebugConnectionPrivate;
    QScopedPointer<QmlDebugClientPrivate> d_ptr;

public:
    static int s_dataStreamVersion;
};

class QMLDEBUG_EXPORT QmlDebugStream : public QDataStream
{
public:
    QmlDebugStream();
    explicit QmlDebugStream(QIODevice *d);
    QmlDebugStream(QByteArray *ba, QIODevice::OpenMode flags);
    QmlDebugStream(const QByteArray &ba);
};

} // namespace QmlDebug

#endif // QMLDEBUGCLIENT_H
