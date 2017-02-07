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

void FindNodesForFileVisitor::visitFolderNode(FolderNode *node)
{
    if (node->filePath() == m_path)
        m_nodes << node;
    foreach (FileNode *fileNode, node->fileNodes()) {
        if (fileNode->filePath() == m_path)
            m_nodes << fileNode;
    }
}

/*!
  \class FindAllFilesVisitor

  Collects file information from all sub file nodes.
 */

Utils::FileNameList FindAllFilesVisitor::filePaths() const
{
    return m_filePaths;
}

void FindAllFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    m_filePaths.append(folderNode->filePath());
    foreach (const FileNode *fileNode, folderNode->fileNodes())
        m_filePaths.append(fileNode->filePath());
}

NodesVisitor::~NodesVisitor()
{
}
