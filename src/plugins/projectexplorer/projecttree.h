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
    static void focusChanged();
    static Project *projectForNode(Node *node);

signals:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);

private:
    void updateFromProjectTreeWidget(Internal::ProjectTreeWidget *widget);
    void documentManagerCurrentFileChanged();
    void updateFromDocumentManager(bool invalidCurrentNode = false);
    void update(Node *node, Project *project);
    void updateContext();

    // slots to kepp the current node/current project from deletion
    void foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode *> &list);
    void foldersRemoved();
    void filesAboutToBeRemoved(FolderNode *, const QList<FileNode *> &list);
    void filesRemoved();
    void aboutToRemoveProject(Project *project);
    void projectRemoved();

    void nodesAdded();
    void updateFromFocus(bool invalidCurrentNode = false);

    void updateExternalFileWarning();
    static bool hasFocus(Internal::ProjectTreeWidget *widget);


    static ProjectTree *s_instance;
    QList<Internal::ProjectTreeWidget *> m_projectTreeWidgets;
    Node *m_currentNode;
    Project *m_currentProject;
    bool m_resetCurrentNodeFolder;
    bool m_resetCurrentNodeFile;
    bool m_resetCurrentNodeProject;
    Core::Context m_lastProjectContext;
};
}

#endif // PROJECTTREE_H
