// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace GenericProjectManager {
namespace Internal {

class GenericProjectPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GenericProjectManager.json")

public:
    ~GenericProjectPlugin() override;

private:
    void initialize() override;
    void extensionsInitialized() override { }

    class GenericProjectPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace GenericProject
