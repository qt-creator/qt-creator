/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlpreviewfileontargetfinder.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <resourceeditor/resourcenode.h>

#include <utils/qtcassert.h>

namespace QmlPreview {
namespace Internal {

void QmlPreviewFileOnTargetFinder::setTarget(ProjectExplorer::Target *target)
{
    m_target = target;
}

ProjectExplorer::Target *QmlPreviewFileOnTargetFinder::target() const
{
    return m_target.data();
}

QString resourceNodePath(const ProjectExplorer::Node *node)
{
    if (auto resourceNode = dynamic_cast<const ResourceEditor::ResourceFileNode *>(node))
        return ":" + resourceNode->qrcPath();
    return QString();
}

QString QmlPreviewFileOnTargetFinder::findPath(const QString &filePath, bool *success) const
{
    if (success)
        *success = (m_target != nullptr);

    if (!m_target)
        return filePath;

    ProjectExplorer::DeployableFile file
            = m_target->deploymentData().deployableForLocalFile(Utils::FilePath::fromString(filePath));
    if (file.isValid())
        return file.remoteFilePath();

    // Try the current node first. It's likely that this is the one we're looking for and if there
    // is any ambiguity (same file mapped to multiple qrc paths) it should take precedence.
    ProjectExplorer::Node *currentNode = ProjectExplorer::ProjectTree::currentNode();
    if (currentNode && currentNode->filePath().toString() == filePath) {
        const QString path = resourceNodePath(currentNode);
        if (!path.isEmpty())
            return path;
    }

    if (ProjectExplorer::Project *project = m_target->project()) {
        if (ProjectExplorer::ProjectNode *rootNode = project->rootProjectNode()) {
            const QList<ProjectExplorer::Node *> nodes = rootNode->findNodes(
                        [&](ProjectExplorer::Node *node) {
                return node->filePath().toString() == filePath;
            });

            for (const ProjectExplorer::Node *node : nodes) {
                const QString path = resourceNodePath(node);
                if (!path.isEmpty())
                    return path;
            }
        } else {
            // Can there be projects without root node?
        }
    } else {
        // Targets should always have a project.
        QTC_CHECK(false);
    }

    if (success) {
        // On desktop, if there is no "remote" path, then the application will load the local path.
        *success = ProjectExplorer::DeviceTypeKitAspect::deviceTypeId(m_target->kit())
                    == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    }
    return filePath;
}

QUrl QmlPreviewFileOnTargetFinder::findUrl(const QString &filePath, bool *success) const
{
    const QString remotePath = findPath(filePath, success);
    if (remotePath.startsWith(':')) {
        QUrl result;
        result.setPath(remotePath.mid(1));
        result.setScheme("qrc");
        return result;
    } else {
        return QUrl::fromLocalFile(remotePath);
    }
}

} // namespace Internal
} // namespace QmlPreview
