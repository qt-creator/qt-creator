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

#include "projectexplorer_export.h"

#include <coreplugin/icontext.h>

#include <functional>

namespace Utils { class FileName; }

namespace ProjectExplorer {
class FileNode;
class FolderNode;
class Node;
class Project;
class ProjectNode;
class SessionNode;

namespace Internal { class ProjectTreeWidget; }

class PROJECTEXPLORER_EXPORT ProjectTree : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTree(QObject *parent = nullptr);
    ~ProjectTree() override;

    static ProjectTree *instance();

    static Project *currentProject();
    static Node *findCurrentNode();
    static Utils::FileName currentFilePath();

    // Integration with ProjectTreeWidget
    static void registerWidget(Internal::ProjectTreeWidget *widget);
    static void unregisterWidget(Internal::ProjectTreeWidget *widget);
    static void nodeChanged(Internal::ProjectTreeWidget *widget);

    static void aboutToShutDown();

    static void showContextMenu(Internal::ProjectTreeWidget *focus, const QPoint &globalPos, Node *node);

    static void highlightProject(Project *project, const QString &message);

    using TreeManagerFunction = std::function<void(FolderNode *)>;
    static void registerTreeManager(const TreeManagerFunction &treeChange);
    static void applyTreeManager(FolderNode *folder);

    // Nodes:
    static bool hasNode(const Node *node);
    static void forEachNode(const std::function<void(Node *)> &task);

    static Project *projectForNode(Node *node);
    static Node *nodeForFile(const Utils::FileName &fileName);

    void collapseAll();

    // for nodes to emit signals, do not call unless you are a node
    static void emitSubtreeChanged(FolderNode *node);

signals:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged();

    // Emitted whenever the model needs to send a update signal.
    void subtreeChanged(ProjectExplorer::FolderNode *node);

    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

    // Emitted on any change to the tree
    void treeChanged();

private:
    void sessionAndTreeChanged();
    void sessionChanged();
    void update();
    void updateFromProjectTreeWidget(Internal::ProjectTreeWidget *widget);
    void updateFromDocumentManager();
    void updateFromNode(Node *node);
    void setCurrent(Node *node, Project *project);
    void updateContext();

    void updateFromFocus();

    void updateExternalFileWarning();
    static bool hasFocus(Internal::ProjectTreeWidget *widget);
    void hideContextMenu();

private:
    static ProjectTree *s_instance;
    QList<QPointer<Internal::ProjectTreeWidget>> m_projectTreeWidgets;
    QVector<TreeManagerFunction> m_treeManagers;
    Node *m_currentNode = nullptr;
    Project *m_currentProject = nullptr;
    Internal::ProjectTreeWidget *m_focusForContextMenu = nullptr;
    Core::Context m_lastProjectContext;
};

} // namespace ProjectExplorer
