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

#include "qbsproject.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {

ProjectExplorer::FileType fileType(const qbs::ArtifactData &artifact)
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

void setupArtifacts(ProjectExplorer::FolderNode *root, const QList<qbs::ArtifactData> &artifacts)
{
    for (const qbs::ArtifactData &ad : artifacts) {
        const Utils::FileName path = Utils::FileName::fromString(ad.filePath());
        const ProjectExplorer::FileType type = fileType(ad);
        const bool isGenerated = ad.isGenerated();

        // A list of human-readable file types that we can reasonably expect
        // to get generated during a build. Extend as needed.
        static const QSet<QString> sourceTags = {
            QLatin1String("c"), QLatin1String("cpp"), QLatin1String("hpp"),
            QLatin1String("objc"), QLatin1String("objcpp"),
            QLatin1String("c_pch_src"), QLatin1String("cpp_pch_src"),
            QLatin1String("objc_pch_src"), QLatin1String("objcpp_pch_src"),
            QLatin1String("asm"), QLatin1String("asm_cpp"),
            QLatin1String("linkerscript"),
            QLatin1String("qrc"), QLatin1String("java.java")
        };
        ProjectExplorer::FileNode * const node
                = new ProjectExplorer::FileNode(path, type, isGenerated);
        node->setListInProject(!isGenerated || ad.fileTags().toSet().intersects(sourceTags));
        root->addNestedNode(node);
    }
    root->compress();
}

QbsProjectManager::Internal::QbsGroupNode
*buildGroupNodeTree(const qbs::GroupData &grp, const QString &productPath, bool productIsEnabled)
{
    QTC_ASSERT(grp.isValid(), return nullptr);

    auto result = new QbsProjectManager::Internal::QbsGroupNode(grp, productPath);

    result->setEnabled(productIsEnabled && grp.isEnabled());
    result->setAbsoluteFilePathAndLine(
                Utils::FileName::fromString(grp.location().filePath()).parentDir(), -1);
    result->setDisplayName(grp.name());
    result->addNode(new QbsProjectManager::Internal::QbsFileNode(
                        Utils::FileName::fromString(grp.location().filePath()),
                        ProjectExplorer::FileType::Project, false,
                        grp.location().line()));

    ::setupArtifacts(result, grp.allSourceArtifacts());

    return result;
}

void setupQbsProductData(QbsProjectManager::Internal::QbsProductNode *node,
                         const qbs::ProductData &prd, const qbs::Project &project)
{
    using namespace QbsProjectManager::Internal;

    node->setEnabled(prd.isEnabled());

    node->setDisplayName(QbsProject::productDisplayName(project, prd));
    node->setAbsoluteFilePathAndLine(Utils::FileName::fromString(prd.location().filePath()).parentDir(), -1);
    const QString &productPath = QFileInfo(prd.location().filePath()).absolutePath();

    // Add QbsFileNode:
    node->addNode(new QbsFileNode(Utils::FileName::fromString(prd.location().filePath()),
                                  ProjectExplorer::FileType::Project, false,
                                  prd.location().line()));


    foreach (const qbs::GroupData &grp, prd.groups()) {
        if (grp.name() == prd.name() && grp.location() == prd.location()) {
            // Set implicit product group right onto this node:
            setupArtifacts(node, grp.allSourceArtifacts());
            continue;
        }
        node->addNode(buildGroupNodeTree(grp, productPath, prd.isEnabled()));
    }

    // Add "Generated Files" Node:
    auto genFiles
            = new ProjectExplorer::VirtualFolderNode(node->filePath(),
                                                     ProjectExplorer::Node::DefaultProjectFilePriority - 10);
    genFiles->setDisplayName(QCoreApplication::translate("QbsProductNode", "Generated files"));
    node->addNode(genFiles);
    setupArtifacts(genFiles, prd.generatedArtifacts());
}

QbsProjectManager::Internal::QbsProductNode *
buildProductNodeTree(const qbs::Project &project, const qbs::ProductData &prd)
{
    auto result = new QbsProjectManager::Internal::QbsProductNode(prd);

    setupQbsProductData(result, prd, project);
    return result;
}

void setupProjectNode(QbsProjectManager::Internal::QbsProjectNode *node, const qbs::ProjectData &prjData,
                      const qbs::Project &qbsProject)
{
    using namespace QbsProjectManager::Internal;
    node->addNode(new QbsFileNode(Utils::FileName::fromString(prjData.location().filePath()),
                                  ProjectExplorer::FileType::Project, false,
                                  prjData.location().line()));
    foreach (const qbs::ProjectData &subData, prjData.subProjects()) {
        auto subProject =
                new QbsProjectManager::Internal::QbsProjectNode(
                    Utils::FileName::fromString(subData.location().filePath()).parentDir());
        setupProjectNode(subProject, subData, qbsProject);
        node->addNode(subProject);
    }

    foreach (const qbs::ProductData &prd, prjData.products())
        node->addNode(buildProductNodeTree(qbsProject, prd));

    if (!prjData.name().isEmpty())
        node->setDisplayName(prjData.name());
    else
        node->setDisplayName(node->project()->displayName());

    node->setProjectData(prjData);
}

QSet<QString> referencedBuildSystemFiles(const qbs::ProjectData &data)
{
    QSet<QString> result;
    result.insert(data.location().filePath());
    foreach (const qbs::ProjectData &subProject, data.subProjects())
        result.unite(referencedBuildSystemFiles(subProject));
    foreach (const qbs::ProductData &product, data.products()) {
        result.insert(product.location().filePath());
        foreach (const qbs::GroupData &group, product.groups())
            result.insert(group.location().filePath());
    }

    return result;
}

QStringList unreferencedBuildSystemFiles(const qbs::Project &p)
{
    QStringList result;
    if (!p.isValid())
        return result;

    const std::set<QString> &available = p.buildSystemFiles();
    QList<QString> referenced = referencedBuildSystemFiles(p.projectData()).toList();
    Utils::sort(referenced);
    std::set_difference(available.begin(), available.end(), referenced.begin(), referenced.end(),
                        std::back_inserter(result));
    return result;
}

} // namespace

namespace QbsProjectManager {
namespace Internal {

QbsRootProjectNode *QbsNodeTreeBuilder::buildTree(QbsProject *project)
{
    if (!project->qbsProjectData().isValid())
        return nullptr;

    auto root = new QbsRootProjectNode(project);
    setupProjectNode(root, project->qbsProjectData(), project->qbsProject());
    auto buildSystemFiles
            = new ProjectExplorer::FolderNode(project->projectDirectory(),
                                              ProjectExplorer::NodeType::Folder,
                                              QCoreApplication::translate("QbsRootProjectNode", "Qbs files"));

    Utils::FileName base = project->projectDirectory();
    const QStringList &files = unreferencedBuildSystemFiles(project->qbsProject());
    for (const QString &f : files) {
        const Utils::FileName filePath = Utils::FileName::fromString(f);
        if (filePath.isChildOf(base))
            buildSystemFiles->addNestedNode(new ProjectExplorer::FileNode(filePath, ProjectExplorer::FileType::Project, false));
    }
    buildSystemFiles->compress();
    root->addNode(buildSystemFiles);

    return root;
}

} // namespace Internal
} // namespace QbsProjectManager
