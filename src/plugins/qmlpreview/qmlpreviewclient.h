// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreview_global.h"
#include <qmldebug/qmldebugclient.h>

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
        Fps
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

    void messageReceived(const QByteArray &message) override;
    void stateChanged(State state) override;

signals:
    void pathRequested(const QString &path);
    void errorReported(const QString &error);
    void fpsReported(const FpsInfo &fpsInfo);
    void debugServiceUnavailable();
};

} // namespace QmlPreview

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewClient::FpsInfo)
