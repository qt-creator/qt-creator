/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    void outlineSortingChanged(bool sort);
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
