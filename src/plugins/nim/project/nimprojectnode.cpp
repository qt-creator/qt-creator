/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimprojectnode.h"
#include "nimproject.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimProjectNode::NimProjectNode(NimProject &project,
                               const FileName &projectFilePath)
    : ProjectNode(projectFilePath)
    , m_project(project)
{}

bool NimProjectNode::supportsAction(ProjectAction action, const Node *node) const
{
    switch (node->nodeType()) {
    case NodeType::File:
        return action == ProjectAction::Rename
            || action == ProjectAction::RemoveFile;
    case NodeType::Folder:
    case NodeType::Project:
        return action == ProjectAction::AddNewFile
            || action == ProjectAction::RemoveFile
            || action == ProjectAction::AddExistingFile;
    default:
        return ProjectNode::supportsAction(action, node);
    }
}

bool NimProjectNode::addFiles(const QStringList &filePaths, QStringList *)
{
    return m_project.addFiles(filePaths);
}

bool NimProjectNode::removeFiles(const QStringList &filePaths, QStringList *)
{
    return m_project.removeFiles(filePaths);
}

bool NimProjectNode::deleteFiles(const QStringList &)
{
    return true;
}

bool NimProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project.renameFile(filePath, newFilePath);
}

}
