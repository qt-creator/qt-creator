// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewclient.h"
#include <qmldebug/qpacketprotocol.h>

#include <QUrl>

namespace QmlPreview {

QmlPreviewClient::QmlPreviewClient(QmlDebug::QmlDebugConnection *connection) :
    QmlDebug::QmlDebugClient(QLatin1String("QmlPreview"), connection)
{
}

void QmlPreviewClient::loadUrl(const QUrl &url)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Load) << url;
    sendMessage(packet.data());
}

void QmlPreviewClient::rerun()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Rerun);
    sendMessage(packet.data());
}

void QmlPreviewClient::zoom(float zoomFactor)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Zoom) << zoomFactor;
    sendMessage(packet.data());
}

void QmlPreviewClient::announceFile(const QString &path, const QByteArray &contents)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(File) << path << contents;
    sendMessage(packet.data());
}

void QmlPreviewClient::announceDirectory(const QString &path, const QStringList &entries)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Directory) << path << entries;
    sendMessage(packet.data());
}

void QmlPreviewClient::announceError(const QString &path)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Error) << path;
    sendMessage(packet.data());
}

void QmlPreviewClient::clearCache()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(ClearCache);
    sendMessage(packet.data());
}

void QmlPreviewClient::messageReceived(const QByteArray &data)
{
    QmlDebug::QPacket packet(dataStreamVersion(), data);
    qint8 command;
    packet >> command;
    switch (command) {
    case Request: {
        QString path;
        packet >> path;
        emit pathRequested(path);
        break;
    }
    case Error: {
        QString error;
        packet >> error;
        emit errorReported(error);
        break;
    }
    case Fps: {
        FpsInfo info;
        packet >> info.numSyncs >> info.minSync >> info.maxSync >> info.totalSync
               >> info.numRenders >> info.minRender >> info.maxRender >> info.totalRender;
        emit fpsReported(info);
        break;
    }
    default:
        qDebug() << "invalid command" << command;
        break;
    }
}

void QmlPreviewClient::stateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state == Unavailable)
        emit debugServiceUnavailable();
}

} // namespace QmlPreview
