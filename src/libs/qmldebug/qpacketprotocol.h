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
