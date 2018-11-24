/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "fakedebugserver.h"
#include <qmldebug/qpacketprotocol.h>

namespace QmlProfiler {
namespace Internal {

void fakeDebugServer(QIODevice *socket)
{
    auto protocol = new QmlDebug::QPacketProtocol(socket, socket);
    QObject::connect(protocol, &QmlDebug::QPacketProtocol::readyRead, [protocol]() {
        QmlDebug::QPacket packet(QDataStream::Qt_4_7);
        const int messageId = 0;
        const int protocolVersion = 1;
        const QStringList pluginNames({"CanvasFrameRate", "EngineControl", "DebugMessages"});
        const QList<float> pluginVersions({1.0f, 1.0f, 1.0f});

        packet << QString::fromLatin1("QDeclarativeDebugClient") << messageId << protocolVersion
               << pluginNames << pluginVersions << QDataStream::Qt_DefaultCompiledVersion;
        protocol->send(packet.data());
        protocol->disconnect();
        protocol->deleteLater();
    });
}

} // namespace Internal
} // namespace QmlProfiler
