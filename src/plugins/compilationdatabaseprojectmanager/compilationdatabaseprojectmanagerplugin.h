// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace CompilationDatabaseProjectManager::Internal {

class CompilationDatabaseProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilationDatabaseProjectManager.json")

    ~CompilationDatabaseProjectManagerPlugin();

    void initialize() final;

    class CompilationDatabaseProjectManagerPluginPrivate *d = nullptr;
};

} // CompilationDatabaseProjectManager::Internal
