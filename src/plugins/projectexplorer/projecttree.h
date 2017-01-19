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
    ~ProjectTree();

    static ProjectTree *instance();

    static Project *currentProject();
    static Node *currentNode();

    // Integration with ProjectTreeWidget
    static void registerWidget(Internal::ProjectTreeWidget *widget);
    static void unregisterWidget(Internal::ProjectTreeWidget *widget);
    static void nodeChanged(Internal::ProjectTreeWidget *widget);

    static void aboutToShutDown();

    static void showContextMenu(Internal::ProjectTreeWidget *focus, const QPoint &globalPos, Node *node);

    static void highlightProject(Project *project, const QString &message);

signals:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);

    // Emitted whenever the model needs to send a update signal.
    void nodeUpdated(ProjectExplorer::Node *node);
    void dataChanged();

    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

public: // for nodes to emit signals, do not call unless you are a node
    static void emitNodeUpdated(ProjectExplorer::Node *node);
    static void emitDataChanged();
    void collapseAll();

private:
    void sessionChanged();
    void focusChanged();
    void updateFromProjectTreeWidget(Internal::ProjectTreeWidget *widget);
    void documentManagerCurrentFileChanged();
    void updateFromDocumentManager(bool invalidCurrentNode = false);
    void updateFromNode(Node *node);
    void update(Node *node, Project *project);
    void updateContext();

    void updateFromFocus(bool invalidCurrentNode = false);

    void updateExternalFileWarning();
    static bool hasFocus(Internal::ProjectTreeWidget *widget);
    void hideContextMenu();
    bool isInNodeHierarchy(Node *n);

private:
    static ProjectTree *s_instance;
    QList<QPointer<Internal::ProjectTreeWidget>> m_projectTreeWidgets;
    QPointer<Node> m_currentNode;
    Project *m_currentProject = nullptr;
    Internal::ProjectTreeWidget *m_focusForContextMenu = nullptr;
    Core::Context m_lastProjectContext;
};

} // namespace ProjectExplorer
