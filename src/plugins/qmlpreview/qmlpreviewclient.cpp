/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

void QmlPreviewClient::language(const QUrl &context, const QString &locale)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(Language) << context << locale;
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
