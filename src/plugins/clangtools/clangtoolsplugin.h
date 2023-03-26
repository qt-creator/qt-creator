// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Core { class IDocument; }
namespace ProjectExplorer { class ProjectPanelFactory; }

namespace ClangTools::Internal {

ProjectExplorer::ProjectPanelFactory *projectPanelFactory();

class ClangToolsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangTools.json")

public:
    ClangToolsPlugin() = default;
    ~ClangToolsPlugin() final;

private:
    void initialize() final;
    void registerAnalyzeActions();
    void onCurrentEditorChanged();

    class ClangToolsPluginPrivate *d = nullptr;
};

} // ClangTools::Internal
