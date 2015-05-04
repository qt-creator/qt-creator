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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTTREE_H
#define PROJECTTREE_H

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
    explicit ProjectTree(QObject *parent = 0);

    static ProjectTree *instance();

    static Project *currentProject();
    static Node *currentNode();

    // Integration with ProjectTreeWidget
    static void registerWidget(Internal::ProjectTreeWidget *widget);
    static void unregisterWidget(Internal::ProjectTreeWidget *widget);
    static void nodeChanged(Internal::ProjectTreeWidget *widget);
    static Project *projectForNode(Node *node);

    static void aboutToShutDown();

    static void showContextMenu(Internal::ProjectTreeWidget *focus, const QPoint &globalPos, Node *node);

signals:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);

    // Emitted whenever the model needs to send a update signal.
    void nodeUpdated(ProjectExplorer::Node *node);

    // projects
    void aboutToChangeShowInSimpleTree(ProjectExplorer::FolderNode*);
    void showInSimpleTreeChanged(ProjectExplorer::FolderNode *node);

    // folders & projects
    void foldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*> &staleFolders);
    void foldersRemoved();

    // files
    void filesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*> &staleFiles);
    void filesRemoved();
    void nodeSortKeyAboutToChange(Node *node);
    void nodeSortKeyChanged();

    void aboutToShowContextMenu(ProjectExplorer::Project *project,
                                ProjectExplorer::Node *node);

public: // for nodes to emit signals, do not call unless you are a node
    void emitNodeUpdated(ProjectExplorer::Node *node);

    // projects
    void emitAboutToChangeShowInSimpleTree(ProjectExplorer::FolderNode *node);
    void emitShowInSimpleTreeChanged(ProjectExplorer::FolderNode *node);

    // folders & projects
    void emitFoldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*> &newFolders);
    void emitFoldersAdded(FolderNode *folder);

    void emitFoldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*> &staleFolders);
    void emitFoldersRemoved(FolderNode *folder);

    // files
    void emitFilesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*> &newFiles);
    void emitFilesAdded(FolderNode *folder);

    void emitFilesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*> &staleFiles);
    void emitFilesRemoved(FolderNode *folder);
    void emitNodeSortKeyAboutToChange(Node *node);
    void emitNodeSortKeyChanged(Node *node);

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
    QList<Internal::ProjectTreeWidget *> m_projectTreeWidgets;
    Node *m_currentNode;
    Project *m_currentProject;
    QList<FileNode *> m_filesAdded;
    QList<FolderNode *> m_foldersAdded;
    bool m_resetCurrentNodeFolder;
    bool m_resetCurrentNodeFile;
    bool m_resetCurrentNodeProject;
    Internal::ProjectTreeWidget *m_focusForContextMenu;
    Core::Context m_lastProjectContext;
};
}

#endif // PROJECTTREE_H
