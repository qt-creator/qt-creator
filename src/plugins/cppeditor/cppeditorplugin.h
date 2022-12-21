// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

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

    static const QStringList &headerSearchPaths();
    static const QStringList &sourceSearchPaths();
    static const QStringList &headerPrefixes();
    static const QStringList &sourcePrefixes();
    static void clearHeaderSourceCache();
    static Utils::FilePath licenseTemplatePath();
    static QString licenseTemplate();
    static bool usePragmaOnce();

    void openDeclarationDefinitionInNextSplit();
    void openTypeHierarchy();
    void openIncludeHierarchy();
    void showPreProcessorDialog();
    void renameSymbolUnderCursor();
    void switchDeclarationDefinition();

    CppCodeModelSettings *codeModelSettings();
    static CppFileSettings *fileSettings();

signals:
    void typeHierarchyRequested();
    void includeHierarchyRequested();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;
    QVector<QObject *> createTestObjects() const override;

    CppEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace CppEditor
