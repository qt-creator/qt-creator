/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    explicit QPacketProtocol(QIODevice *dev, QObject *parent = 0);

    void send(const QByteArray &data);
    qint64 packetsAvailable() const;
    QByteArray read();
    bool waitForReadyRead(int msecs = 3000);

signals:
    void readyRead();
    void invalidPacket();

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
