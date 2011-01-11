/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEDEBUGCLIENT_H
#define QDECLARATIVEDEBUGCLIENT_H

#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_HEADER

namespace QmlJsDebugClient {

class QDeclarativeDebugConnectionPrivate;
class QDeclarativeDebugConnection : public QTcpSocket
{
    Q_OBJECT
    Q_DISABLE_COPY(QDeclarativeDebugConnection)
public:
    QDeclarativeDebugConnection(QObject * = 0);
    ~QDeclarativeDebugConnection();

    bool isConnected() const;
private:
    QDeclarativeDebugConnectionPrivate *d;
    friend class QDeclarativeDebugClient;
    friend class QDeclarativeDebugClientPrivate;
};

class QDeclarativeDebugClientPrivate;
class QDeclarativeDebugClient : public QObject
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

    void sendMessage(const QByteArray &);

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    friend class QDeclarativeDebugConnection;
    friend class QDeclarativeDebugConnectionPrivate;
    QScopedPointer<QDeclarativeDebugClientPrivate> d_ptr;
};

}

QT_END_HEADER

#endif // QDECLARATIVEDEBUGCLIENT_H
