// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }
namespace Utils { class FilePath; }

namespace CppEditor {
class CppCodeModelSettings;

namespace Internal {

class CppEditorPluginPrivate;
class CppFileSettings;
class CppQuickFixAssistProvider;

class CppEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    CppEditorPlugin();
    ~CppEditorPlugin() override;

    static CppEditorPlugin *instance();

    CppQuickFixAssistProvider *quickFixProvider() const;

    static void clearHeaderSourceCache();
    static Utils::FilePath licenseTemplatePath(ProjectExplorer::Project *project);
    static QString licenseTemplate(ProjectExplorer::Project *project);
    static bool usePragmaOnce(ProjectExplorer::Project *project);

    void openDeclarationDefinitionInNextSplit();
    void openTypeHierarchy();
    void openIncludeHierarchy();
    void showPreProcessorDialog();
    void renameSymbolUnderCursor();
    void switchDeclarationDefinition();

    CppCodeModelSettings *codeModelSettings();
    static CppFileSettings fileSettings(ProjectExplorer::Project *project);
#ifdef WITH_TESTS
    static void setGlobalFileSettings(const CppFileSettings &settings);
#endif

signals:
    void typeHierarchyRequested();
    void includeHierarchyRequested();

private:
    void initialize() override;
    void extensionsInitialized() override;

    void setupMenus();
    void addPerSymbolActions();
    void addActionsForSelections();
    void addPerFileActions();
    void addGlobalActions();
    void setupProjectPanels();
    void registerVariables();
    void registerTests();

    CppEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace CppEditor
