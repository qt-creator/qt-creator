// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Python::Internal {

class PythonPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Python.json")

public:
    PythonPlugin();
    ~PythonPlugin() final;

    static PythonPlugin *instance();

private:
    void initialize() final;
    void extensionsInitialized() final;

    class PythonPluginPrivate *d = nullptr;
};

} // Python::Internal
