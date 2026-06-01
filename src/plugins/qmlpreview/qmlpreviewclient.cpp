// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewclient.h"
#include <qmldebug/qpacketprotocol.h>

#include <QUrl>

using namespace QmlDebug;
namespace QmlPreview {

QmlPreviewClient::QmlPreviewClient(QmlDebug::QmlDebugConnection *connection)
    : QmlDebug::QmlDebugClient(QLatin1String("QmlPreview"), connection)
{
    connect(this, &QmlPreviewClient::configure, this, &QmlPreviewClient::announceConfiguration);
    connect(this, &QmlPreviewClient::confirmation, this, [this](const Settings &settings) {
        m_confirmedSettings = settings;
        configureEventReplay();
    });
}

void QmlPreviewClient::loadUrl(const QUrl &url)
{
    m_numExpectedEvents = m_events.size();
    if (m_numExpectedEvents > 0 && m_replayClient
        && m_replayClient->state() == QmlDebugClient::Enabled) {
        replayEventsForUrl(url, m_events, m_eventTypes);
    } else {
        m_events.clear();
        doLoad(url);
    }
}

void QmlPreviewClient::doLoad(const QUrl &url)
{
    QPacket packet(dataStreamVersion());
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

void QmlPreviewClient::announceConfiguration()
{
    QPacket packet(dataStreamVersion());
    // We always want in-place updates but this should be confirmed by the server
    // and then the configured setting is written to m_confirmedSettings.
    // This allows the server to disable in-place updates if it doesn't support them.
    packet << static_cast<qint8>(Configuration) << true;
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
    case Confirmation: {
        Settings settings;
        packet >> settings.enableInPlaceUpdates;
        emit confirmation(settings);
        break;
    }
    case HotReloadFailure: {
        QString reason;
        packet >> reason;
        emit hotReloadFailure(reason);
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
    else if (state == Enabled)
        emit configure();
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

void QmlPreviewClient::configureEventReplay()
{
    m_recordClient.reset(new QmlProfilerTraceClient(
        connection(),
        std::bind(&QmlPreviewClient::appendEventType, this, std::placeholders::_1),
        std::bind(&QmlPreviewClient::appendEvent, this, std::placeholders::_1),
        1ull << ProfileInputEvents));

    m_recordClient->setFlushInterval(1);
    m_recordClient->setRecording(true);

    m_replayClient.reset(new QuickEventReplayClient(connection()));

    m_replayTimer.setInterval(100);
    connect(&m_replayTimer, &QTimer::timeout, this, [this]() {
        if (m_events.size() < m_numExpectedEvents)
            return;
        setAnimationSpeed(1);
        m_replayTimer.stop();
    });

    // We want to start the replay as soon as possible after the configuration is confirmed.
    if (m_events.size() > 0 && m_replayClient && m_replayClient->state() == QmlDebugClient::Enabled)
        replayEventsForUrl(QUrl(), m_events, m_eventTypes);
}

void QmlPreviewClient::replayEventsForUrl(
    const QUrl &url, QList<QmlDebug::QmlEvent> &events, QList<QmlDebug::QmlEventType> &types)
{
    const QList<QmlEvent> recorded = std::exchange(events, {});
    setAnimationSpeed(1000);
    doLoad(url);
    for (const QmlEvent &event : recorded)
        m_replayClient->sendEvent(types[event.typeIndex()], event);
    m_replayTimer.start();
}

void QmlPreviewClient::setEvents(const QList<QmlDebug::QmlEvent> &events)
{
    m_events = events;
}

void QmlPreviewClient::setEventTypes(const QList<QmlDebug::QmlEventType> &types)
{
    m_eventTypes = types;
}

} // namespace QmlPreview
