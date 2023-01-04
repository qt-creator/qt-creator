// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebug_global.h"

#include <qobject.h>
#include <qdatastream.h>
#include <qbuffer.h>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace QmlDebug {

class QPacketProtocolPrivate;
class QMLDEBUG_EXPORT QPacketProtocol : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPacketProtocol)
public:
    explicit QPacketProtocol(QIODevice *dev, QObject *parent = nullptr);

    void send(const QByteArray &data);
    qint64 packetsAvailable() const;
    QByteArray read();
    bool waitForReadyRead(int msecs = 3000);

signals:
    void readyRead();
    void protocolError();

private:
    QPacketProtocolPrivate *d;
};

class QMLDEBUG_EXPORT QPacket : public QDataStream
{
public:
    QPacket(int version);
    explicit QPacket(int version, const QByteArray &ba);
    QByteArray data() const;

private:
    QBuffer buf;
};

} // QmlDebug
