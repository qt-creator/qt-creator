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

#include "qmlpreviewconnectionmanager.h"
#include "qmlpreviewfileontargetfinder.h"
#include "qmlpreviewplugin.h"
#include <projectexplorer/runconfiguration.h>

namespace QmlPreview {

class QmlPreviewRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlPreviewRunner(ProjectExplorer::RunControl *runControl, QmlPreviewFileLoader fileLoader,
                     QmlPreviewFileClassifier fileClassifier, QmlPreviewFpsHandler fpsHandler,
                     float initialZoom, const QString &initialLocale);

    void setServerUrl(const QUrl &serverUrl);
    QUrl serverUrl() const;

signals:
    void loadFile(const QString &previewedFile, const QString &changedFile,
                  const QByteArray &contents);
    void language(const QString &locale);
    void zoom(float zoomFactor);
    void rerun();
    void ready();

private:
    void start() override;
    void stop() override;

    QScopedPointer<Internal::QmlPreviewConnectionManager> m_connectionManager;
};

class LocalQmlPreviewSupport : public ProjectExplorer::SimpleTargetRunner
{
    Q_OBJECT

public:
    LocalQmlPreviewSupport(ProjectExplorer::RunControl *runControl);
};

} // namespace QmlPreview
