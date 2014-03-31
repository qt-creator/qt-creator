/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RESOURCEEDITORPLUGIN_H
#define RESOURCEEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Node;
class Project;
}

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

    void openEditorContextMenu();
    void openTextEditorContextMenu();

    void copyPathContextMenu();
    void copyUrlContextMenu();

    void updateContextActions(ProjectExplorer::Node*,ProjectExplorer::Project*);

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

    QAction *m_renameResourceFile;
    QAction *m_removeResourceFile;

    QAction *m_openInEditor;
    QAction *m_openInTextEditor;

    // file context menu
    Utils::ParameterAction *m_copyPath;
    Utils::ParameterAction *m_copyUrl;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RESOURCEEDITORPLUGIN_H
