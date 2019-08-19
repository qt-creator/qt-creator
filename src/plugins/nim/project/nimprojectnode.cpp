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

#include "nimbuildsystem.h"

#include <projectexplorer/projecttree.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimProjectNode::NimProjectNode(const FilePath &projectFilePath)
    : ProjectNode(projectFilePath)
{}

bool NimProjectNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (node->asFileNode()) {
        return action == ProjectAction::Rename
            || action == ProjectAction::RemoveFile;
    }
    if (node->isFolderNodeType() || node->isProjectNodeType()) {
        return action == ProjectAction::AddNewFile
            || action == ProjectAction::RemoveFile
            || action == ProjectAction::AddExistingFile;
    }
    return ProjectNode::supportsAction(action, node);
}

bool NimProjectNode::addFiles(const QStringList &filePaths, QStringList *)
{
    return buildSystem()->addFiles(filePaths);
}

RemovedFilesFromProject NimProjectNode::removeFiles(const QStringList &filePaths,
                                                    QStringList *)
{
    return buildSystem()->removeFiles(filePaths) ? RemovedFilesFromProject::Ok
                                                 : RemovedFilesFromProject::Error;
}

bool NimProjectNode::deleteFiles(const QStringList &)
{
    return true;
}

bool NimProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return buildSystem()->renameFile(filePath, newFilePath);
}

NimBuildSystem *NimProjectNode::buildSystem() const
{
    return qobject_cast<NimBuildSystem *>(
        ProjectTree::instance()->projectForNode(this)->buildSystem());
}

} // namespace Nim
