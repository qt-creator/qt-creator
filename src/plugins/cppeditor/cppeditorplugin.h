// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }
namespace Utils { class FilePath; }

namespace CppEditor::Internal {

class CppEditorPluginPrivate;

class CppEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    CppEditorPlugin();
    ~CppEditorPlugin() override;

    static CppEditorPlugin *instance();

    static void clearHeaderSourceCache();

    void openDeclarationDefinitionInNextSplit();
    void openIncludeHierarchy();
    void showPreProcessorDialog();
    void renameSymbolUnderCursor();
    void switchDeclarationDefinition();

signals:
    void includeHierarchyRequested();

private:
    void initialize() override;
    void extensionsInitialized() override;

    void setupMenus();
    void addPerSymbolActions();
    void addActionsForSelections();
    void addPerFileActions();
    void addGlobalActions();
    void registerVariables();
    void registerTests();

    CppEditorPluginPrivate *d = nullptr;
};

} // namespace CppEditor::Internal
