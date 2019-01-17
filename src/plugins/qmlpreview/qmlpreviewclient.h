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
        Fps,
        Language
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
    void language(const QUrl &context, const QString &locale);
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
