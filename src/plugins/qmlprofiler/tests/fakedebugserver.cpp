// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
