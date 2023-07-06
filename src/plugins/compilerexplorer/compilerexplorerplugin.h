// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace CompilerExplorer::Internal {

class CompilerExplorerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilerExplorer.json")

public:
    CompilerExplorerPlugin();
    ~CompilerExplorerPlugin() override;

    void initialize() override;

    void extensionsInitialized() override;
};

} // namespace CompilerExplorer::Internal
