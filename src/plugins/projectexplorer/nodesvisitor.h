/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef NODESVISITOR_H
#define NODESVISITOR_H

#include "projectexplorer_export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace ProjectExplorer {

class Node;
class FileNode;
class SessionNode;
class ProjectNode;
class FolderNode;

class NodesVisitor {
public:
    virtual ~NodesVisitor() {}

    virtual void visitSessionNode(SessionNode *) {}
    virtual void visitProjectNode(ProjectNode *) {}
    virtual void visitFolderNode(FolderNode *) {}
protected:
     NodesVisitor() {}
};

/* useful visitors */

class PROJECTEXPLORER_EXPORT FindNodesForFileVisitor : public NodesVisitor {
public:
    FindNodesForFileVisitor(const QString &fileToSearch);

    QList<Node*> nodes() const;

    void visitProjectNode(ProjectNode *node);
    void visitFolderNode(FolderNode *node);
private:
    QString m_path;
    QList<Node*> m_nodes;
};

class PROJECTEXPLORER_EXPORT FindAllFilesVisitor : public NodesVisitor {
public:
    QStringList filePaths() const;

    void visitProjectNode(ProjectNode *projectNode);
    void visitFolderNode(FolderNode *folderNode);
private:
    QStringList m_filePaths;
};

} // namespace ProjectExplorer

#endif // NODESVISITOR_H
