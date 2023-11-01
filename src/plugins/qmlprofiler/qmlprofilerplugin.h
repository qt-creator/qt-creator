// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmlProfiler::Internal {

class QmlProfilerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProfiler.json")

private:
    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    class QmlProfilerPluginPrivate *d = nullptr;
};

} // QmlProfiler::Internal
