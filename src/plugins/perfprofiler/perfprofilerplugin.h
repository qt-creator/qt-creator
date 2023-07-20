// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace PerfProfiler::Internal {

class PerfProfilerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PerfProfiler.json")

public:
    ~PerfProfilerPlugin();

    void initialize() final;

    class PerfProfilerPluginPrivate *d = nullptr;
};

} // PerfProfiler::Internal
