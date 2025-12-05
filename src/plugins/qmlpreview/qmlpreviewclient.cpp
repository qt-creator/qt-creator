// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewclient.h"
#include <qmldebug/qpacketprotocol.h>

#include <QUrl>

using namespace QmlDebug;
namespace QmlPreview {

QmlPreviewClient::QmlPreviewClient(QmlDebug::QmlDebugConnection *connection)
    : QmlDebug::QmlDebugClient(QLatin1String("QmlPreview"), connection)
    , m_recordClient(new QmlProfilerTraceClient(
          connection,
          std::bind(&QmlPreviewClient::appendEventType, this, std::placeholders::_1),
          std::bind(&QmlPreviewClient::appendEvent, this, std::placeholders::_1),
          1ull << ProfileInputEvents))
    , m_replayClient(new QuickEventReplayClient(connection))
{
    m_recordClient->setFlushInterval(1);
    m_recordClient->setRecording(true);

    m_replayTimer.setInterval(100);
    connect(&m_replayTimer, &QTimer::timeout, this, [this]() {
        if (m_events.size() < m_numExpectedEvents)
            return;
        setAnimationSpeed(1);
        m_replayTimer.stop();
    });
}

void QmlPreviewClient::loadUrl(const QUrl &url)
{
    const auto doLoad = [&]() {
        QPacket packet(dataStreamVersion());
        packet << static_cast<qint8>(Load) << url;
        sendMessage(packet.data());
    };

    m_numExpectedEvents = m_events.size();
    if (m_numExpectedEvents > 0 && m_replayClient->state() == QmlDebugClient::Enabled) {
        const QList<QmlEvent> recorded = std::exchange(m_events, {});
        setAnimationSpeed(1000);
        doLoad();
        for (const QmlEvent &event : recorded)
            m_replayClient->sendEvent(m_eventTypes[event.typeIndex()], event);
        m_replayTimer.start();
    } else {
        m_events.clear();
        doLoad();
    }
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

void QmlPreviewClient::setAnimationSpeed(float factor)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(AnimationSpeed) << factor;
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

int QmlPreviewClient::appendEventType(QmlDebug::QmlEventType &&type)
{
    const int index = m_eventTypes.size();
    m_eventTypes.append(std::move(type));
    return index;
}

void QmlPreviewClient::appendEvent(QmlDebug::QmlEvent &&event)
{
    // Compress the time stamps so that the events are processed in quick succession.
    event.setTimestamp(m_events.size());
    m_events.append(std::move(event));
}

} // namespace QmlPreview
