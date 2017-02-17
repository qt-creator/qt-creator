/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qbsnodetreebuilder.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {

void setupArtifacts(ProjectExplorer::FolderNode *root, const QList<qbs::ArtifactData> &artifacts)
{
    QList<ProjectExplorer::FileNode *> fileNodes
            = Utils::transform(artifacts, [](const qbs::ArtifactData &ad) {
        const Utils::FileName path = Utils::FileName::fromString(ad.filePath());
        const ProjectExplorer::FileType type =
                QbsProjectManager::Internal::QbsNodeTreeBuilder::fileType(ad);
        const bool isGenerated = ad.isGenerated();
        return new ProjectExplorer::FileNode(path, type, isGenerated);
    });

    root->buildTree(fileNodes);
    root->compress();
}

} // namespace

namespace QbsProjectManager {
namespace Internal {

ProjectExplorer::FileType QbsNodeTreeBuilder::fileType(const qbs::ArtifactData &artifact)
{
    QTC_ASSERT(artifact.isValid(), return ProjectExplorer::FileType::Unknown);

    if (artifact.fileTags().contains("c")
            || artifact.fileTags().contains("cpp")
            || artifact.fileTags().contains("objc")
            || artifact.fileTags().contains("objcpp")) {
        return ProjectExplorer::FileType::Source;
    }
    if (artifact.fileTags().contains("hpp"))
        return ProjectExplorer::FileType::Header;
    if (artifact.fileTags().contains("qrc"))
        return ProjectExplorer::FileType::Resource;
    if (artifact.fileTags().contains("ui"))
        return ProjectExplorer::FileType::Form;
    if (artifact.fileTags().contains("scxml"))
        return ProjectExplorer::FileType::StateChart;
    return ProjectExplorer::FileType::Unknown;
}

QbsGroupNode *QbsNodeTreeBuilder::buildGroupNodeTree(const qbs::GroupData &grp,
                                                     const QString &productPath,
                                                     bool productIsEnabled)
{
    QTC_ASSERT(grp.isValid(), return nullptr);

    auto result = new QbsGroupNode(grp, productPath);

    result->setEnabled(productIsEnabled && grp.isEnabled());
    result->setAbsoluteFilePathAndLine(
                Utils::FileName::fromString(grp.location().filePath()).parentDir(), -1);
    result->setDisplayName(grp.name());
    result->addNode(new QbsFileNode(Utils::FileName::fromString(grp.location().filePath()),
                                    ProjectExplorer::FileType::Project, false,
                                    grp.location().line()));

    ::setupArtifacts(result, grp.allSourceArtifacts());

    return result;
}

void QbsNodeTreeBuilder::setupArtifacts(QbsBaseProjectNode *node, const QList<qbs::ArtifactData> &artifacts)
{
    ::setupArtifacts(node, artifacts);
}

} // namespace Internal
} // namespace QbsProjectManager
