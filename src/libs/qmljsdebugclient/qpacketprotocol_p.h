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

#ifndef QPACKETPROTOCOL_H
#define QPACKETPROTOCOL_H

#include <QtCore/qobject.h>
#include <QtCore/qdatastream.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE
class QIODevice;
class QBuffer;
QT_END_NAMESPACE

namespace QmlJsDebugClient {

class QPacket;
class QPacketAutoSend;

class QPacketProtocolPrivate;

class QPacketProtocol : public QObject
{
Q_OBJECT
public:
    explicit QPacketProtocol(QIODevice * dev, QObject * parent = 0);
    virtual ~QPacketProtocol();

    qint32 maximumPacketSize() const;
    qint32 setMaximumPacketSize(qint32);

    QPacketAutoSend send();
    void send(const QPacket &);

    qint64 packetsAvailable() const;
    QPacket read();

    void clear();

    QIODevice * device();

Q_SIGNALS:
    void readyRead();
    void invalidPacket();
    void packetWritten();

private:
    QPacketProtocolPrivate * d;
};


class QPacket : public QDataStream
{
public:
    QPacket();
    QPacket(const QPacket &);
    virtual ~QPacket();

    void clear();
    bool isEmpty() const;

protected:
    friend class QPacketProtocol;
    QPacket(const QByteArray & ba);
    QByteArray b;
    QBuffer * buf;
};

class QPacketAutoSend : public QPacket
{
public:
    virtual ~QPacketAutoSend();

private:
    friend class QPacketProtocol;
    QPacketAutoSend(QPacketProtocol *);
    QPacketProtocol * p;
};

}

QT_END_HEADER

#endif
