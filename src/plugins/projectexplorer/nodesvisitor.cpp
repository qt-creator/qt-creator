/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nodesvisitor.h"
#include "projectnodes.h"

using namespace ProjectExplorer;

/*!
  \class NodesVisitor

  \brief Base class for visitors that can be used to traverse a node hierarchy.

  The class follows the visitor pattern as described in Gamma et al. Pass
  an instance of NodesVisitor to FolderNode::accept(): The visit functions
  will be called for each node in the subtree, except for file nodes:
  Access these through FolderNode::fileNodes() in visitProjectNode()
  and visitoFolderNode().
*/

/*!
  \fn NodesVisitor::visitSessionNode(SessionNode *)

  Called for the root session node.

  The default implementation does nothing.
  */

/*!
  \fn NodesVisitor::visitProjectNode(SessionNode *)

  Called for a project node.

  The default implementation does nothing.
  */

/*!
  \fn NodesVisitor::visitFolderNode(SessionNode *)

  Called for a folder node that is _not_ a SessionNode or a ProjectNode.

  The default implementation does nothing.
  */


/*!
  \class FindNodeForFileVisitor

  Searches the first node that has the given file as its path.
 */

FindNodesForFileVisitor::FindNodesForFileVisitor(const Utils::FileName &fileToSearch)
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
    if (node->path() == m_path)
        m_nodes << node;
    foreach (FileNode *fileNode, node->fileNodes()) {
        if (fileNode->path() == m_path)
            m_nodes << fileNode;
    }
}

void FindNodesForFileVisitor::visitSessionNode(SessionNode *node)
{
    visitFolderNode(node);
}

/*!
  \class FindAllFilesVisitor

  Collects file information from all sub file nodes.
 */

Utils::FileNameList FindAllFilesVisitor::filePaths() const
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

NodesVisitor::~NodesVisitor()
{
}
