/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QPACKETPROTOCOL_H
#define QPACKETPROTOCOL_H

#include <qobject.h>
#include <qdatastream.h>

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlDebug {

class QPacket;
class QPacketAutoSend;

class QPacketProtocolPrivate;

class QPacketProtocol : public QObject
{
    Q_OBJECT

public:
    explicit QPacketProtocol(QIODevice *dev, QObject *parent = 0);
    virtual ~QPacketProtocol();

    qint32 maximumPacketSize() const;
    qint32 setMaximumPacketSize(qint32);

    QPacketAutoSend send();
    void send(const QPacket &);

    qint64 packetsAvailable() const;
    QPacket read();

    bool waitForReadyRead(int msecs = 3000);

    void clear();

    QIODevice *device();

Q_SIGNALS:
    void readyRead();
    void invalidPacket();
    void packetWritten();

private:
    QPacketProtocolPrivate *d;
};


class QPacket : public QDataStream
{
public:
    QPacket();
    QPacket(const QPacket &);
    virtual ~QPacket();

    void clear();
    bool isEmpty() const;
    QByteArray data() const;

protected:
    friend class QPacketProtocol;
    QPacket(const QByteArray &ba);
    QByteArray b;
    QBuffer *buf;
};

class QPacketAutoSend : public QPacket
{
public:
    virtual ~QPacketAutoSend();

private:
    friend class QPacketProtocol;
    QPacketAutoSend(QPacketProtocol *);
    QPacketProtocol *p;
};

} // QmlDebug

#endif
