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

#include "nodesvisitor.h"
#include "projectnodes.h"

using namespace ProjectExplorer;

/*!
  \class NodesVisitor

  \short Base class for visitors that can be used to traverse a node hierarchy.

  The class follows the visitor pattern as described in Gamma et al. Pass
  an instance of NodesVisitor to FolderNode::accept(): The visit methods
  will be called for each node in the subtree, except for file nodes:
  Access these through FolderNode::fileNodes() in visitProjectNode()
  and visitoFolderNode().
*/

/*!
  \method NodesVisitor::visitSessionNode(SessionNode *)

  Called for the root session node.

  The default implementation does nothing.
  */

/*!
  \method NodesVisitor::visitProjectNode(SessionNode *)

  Called for a project node.

  The default implementation does nothing.
  */

/*!
  \method NodesVisitor::visitFolderNode(SessionNode *)

  Called for a folder node that is _not_ a SessionNode or a ProjectNode.

  The default implementation does nothing.
  */


/*!
  \class FindNodeForFileVisitor

  Searches the first node that has the given file as it's path.
 */

FindNodesForFileVisitor::FindNodesForFileVisitor(const QString &fileToSearch)
    : m_path(fileToSearch)
{
}

QList<Node*> FindNodesForFileVisitor::nodes() const
{
    return m_nodes;
}

void FindNodesForFileVisitor::visitProjectNode(ProjectNode *node)
{
    visitFolderNode(node);
}

void FindNodesForFileVisitor::visitFolderNode(FolderNode *node)
{
    if (node->path() == m_path) {
        m_nodes << node;
    }
    foreach (FileNode *fileNode, node->fileNodes()) {
        if (fileNode->path() == m_path) {
            m_nodes << fileNode;
        }
    }
}

/*!
  \class FindAllFilesVisitor

  Collects file information from all sub file nodes.
 */

QStringList FindAllFilesVisitor::filePaths() const
{
    return m_filePaths;
}

void FindAllFilesVisitor::visitProjectNode(ProjectNode *projectNode)
{
    visitFolderNode(projectNode);
}

void FindAllFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    m_filePaths.append(folderNode->path());
    foreach (const FileNode *fileNode, folderNode->fileNodes())
        m_filePaths.append(fileNode->path());
}
