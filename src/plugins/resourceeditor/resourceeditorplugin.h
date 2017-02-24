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

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Utils { class ParameterAction; }

namespace ResourceEditor {
namespace Internal {

class ResourceEditorW;
class ResourceWizard;
class ResourceEditorFactory;

class ResourceEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ResourceEditor.json")

public:
    ResourceEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

private slots:
    void onUndo();
    void onRedo();
    void onRefresh();

    void addPrefixContextMenu();
    void renamePrefixContextMenu();
    void removePrefixContextMenu();
    void renameFileContextMenu();
    void removeFileContextMenu();
    void removeNonExisting();

    void openEditorContextMenu();

    void copyPathContextMenu();
    void copyUrlContextMenu();

    void updateContextActions();

public:
    void onUndoStackChanged(ResourceEditorW const *editor, bool canUndo, bool canRedo);

private:
    ResourceEditorW * currentEditor() const;

private:
    QAction *m_redoAction;
    QAction *m_undoAction;
    QAction *m_refreshAction;

    // project tree's folder context menu
    QAction *m_addPrefix;
    QAction *m_removePrefix;
    QAction *m_renamePrefix;
    QAction *m_removeNonExisting;

    QAction *m_renameResourceFile;
    QAction *m_removeResourceFile;

    QAction *m_openInEditor;
    QMenu *m_openWithMenu;

    // file context menu
    Utils::ParameterAction *m_copyPath;
    Utils::ParameterAction *m_copyUrl;
};

} // namespace Internal
} // namespace ResourceEditor
