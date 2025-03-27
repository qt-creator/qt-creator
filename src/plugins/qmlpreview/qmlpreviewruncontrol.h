// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreviewplugin.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace QmlPreview {

struct QmlPreviewRunnerSetting
{
    QmlPreviewFileLoader fileLoader;
    QmlPreviewFileClassifier fileClassifier;
    QmlPreviewFpsHandler fpsHandler;
    float zoomFactor = -1.0;
    QmlDebugTranslationClientFactoryFunction createDebugTranslationClientMethod;
    QmlPreviewRefreshTranslationFunction refreshTranslationsFunction;
};

class QmlPreviewRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    QmlPreviewRunWorkerFactory();
};

class LocalQmlPreviewSupportFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    LocalQmlPreviewSupportFactory();
};

} // QmlPreview
