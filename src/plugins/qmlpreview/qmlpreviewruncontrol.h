// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreviewconnectionmanager.h"
#include "qmlpreviewfileontargetfinder.h"
#include "qmlpreviewplugin.h"
#include <projectexplorer/runconfiguration.h>

namespace QmlPreview {

struct QmlPreviewRunnerSetting {
    ProjectExplorer::RunControl *runControl = nullptr;
    QmlPreviewFileLoader fileLoader;
    QmlPreviewFileClassifier fileClassifier;
    QmlPreviewFpsHandler fpsHandler;
    float zoom = 1.0;
    QString language;
    QmlDebugTranslationClientCreator createDebugTranslationClientMethod;
};

class QmlPreviewRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlPreviewRunner(const QmlPreviewRunnerSetting &settings);

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

    Internal::QmlPreviewConnectionManager m_connectionManager;
};

class LocalQmlPreviewSupportFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    LocalQmlPreviewSupportFactory();
};

} // QmlPreview
