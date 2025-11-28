// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreview_global.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmlprofilertraceclient.h>
#include <qmldebug/quickeventreplayclient.h>

#include <QTimer>

namespace QmlPreview {

class QMLPREVIEW_EXPORT QmlPreviewClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    enum Command {
        File,
        Load,
        Request,
        Error,
        Rerun,
        Directory,
        ClearCache,
        Zoom,
        Fps,
        AnimationSpeed,
    };

    struct FpsInfo {
        quint16 numSyncs = 0;
        quint16 minSync = std::numeric_limits<quint16>::max();
        quint16 maxSync = 0;
        quint16 totalSync = 0;

        quint16 numRenders = 0;
        quint16 minRender = std::numeric_limits<quint16>::max();
        quint16 maxRender = 0;
        quint16 totalRender = 0;
    };

    explicit QmlPreviewClient(QmlDebug::QmlDebugConnection *connection);

    void loadUrl(const QUrl &url);
    void rerun();
    void zoom(float zoomFactor);
    void announceFile(const QString &path, const QByteArray &contents);
    void announceDirectory(const QString &path, const QStringList &entries);
    void announceError(const QString &path);
    void clearCache();
    void setAnimationSpeed(float factor);

    void messageReceived(const QByteArray &message) override;
    void stateChanged(State state) override;

#if WITH_TESTS
    void injectEvents(
        const QList<QmlDebug::QmlEventType> &types, const QList<QmlDebug::QmlEvent> events)
    {
        m_eventTypes = types;
        m_events = events;
    }
#endif

signals:
    void pathRequested(const QString &path);
    void errorReported(const QString &error);
    void fpsReported(const FpsInfo &fpsInfo);
    void debugServiceUnavailable();

private:
    int appendEventType(QmlDebug::QmlEventType &&type);
    void appendEvent(QmlDebug::QmlEvent &&event);

    // Use QScopedPointerDeleteLater here. The connection will call stateChanged() on all clients
    // that are alive when it gets disconnected. One way to notice a disconnection is failing to
    // send the plugin advertisement when a client unregisters. If one of the other clients is
    // half-destructed at that point, we get invalid memory accesses. Therefore, we cannot nest the
    // dtor calls.
    std::unique_ptr<QmlDebug::QmlProfilerTraceClient, QScopedPointerDeleteLater> m_recordClient;
    std::unique_ptr<QmlDebug::QuickEventReplayClient, QScopedPointerDeleteLater> m_replayClient;

    QList<QmlDebug::QmlEventType> m_eventTypes;
    QList<QmlDebug::QmlEvent> m_events;

    QTimer m_replayTimer;
    qsizetype m_numExpectedEvents = 0;
};

} // namespace QmlPreview

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewClient::FpsInfo)
