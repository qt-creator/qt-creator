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

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

namespace  {

FileType fileType(const qbs::ArtifactData &artifact)
{
    QTC_ASSERT(artifact.isValid(), return FileType::Unknown);

    const QStringList fileTags = artifact.fileTags();
    if (fileTags.contains("c")
            || fileTags.contains("cpp")
            || fileTags.contains("objc")
            || fileTags.contains("objcpp")) {
        return FileType::Source;
    }
    if (fileTags.contains("hpp"))
        return FileType::Header;
    if (fileTags.contains("qrc"))
        return FileType::Resource;
    if (fileTags.contains("ui"))
        return FileType::Form;
    if (fileTags.contains("scxml"))
        return FileType::StateChart;
    if (fileTags.contains("qt.qml.qml"))
        return FileType::QML;
    return FileType::Unknown;
}

void setupArtifacts(FolderNode *root, const QList<qbs::ArtifactData> &artifacts)
{
    for (const qbs::ArtifactData &ad : artifacts) {
        const FilePath path = FilePath::fromString(ad.filePath());
        const FileType type = fileType(ad);
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
        auto node = std::make_unique<FileNode>(path, type);
        node->setIsGenerated(isGenerated);
        node->setListInProject(!isGenerated || Utils::toSet(ad.fileTags()).intersects(sourceTags));
        root->addNestedNode(std::move(node));
    }
    root->compress();
}

std::unique_ptr<QbsGroupNode>
buildGroupNodeTree(const qbs::GroupData &grp, const QString &productPath, bool productIsEnabled)
{
    QTC_ASSERT(grp.isValid(), return nullptr);

    auto fileNode = std::make_unique<FileNode>(FilePath::fromString(grp.location().filePath()),
                                               FileType::Project);
    fileNode->setLine(grp.location().line());

    auto result = std::make_unique<QbsGroupNode>(grp, productPath);

    result->setEnabled(productIsEnabled && grp.isEnabled());
    result->setAbsoluteFilePathAndLine(
                FilePath::fromString(grp.location().filePath()).parentDir(), -1);
    result->setDisplayName(grp.name());
    result->addNode(std::move(fileNode));

    setupArtifacts(result.get(), grp.allSourceArtifacts());

    return result;
}

void setupQbsProductData(QbsProductNode *node, const qbs::ProductData &prd)
{
    auto fileNode = std::make_unique<FileNode>(FilePath::fromString(prd.location().filePath()),
                                               FileType::Project);
    fileNode->setLine(prd.location().line());

    node->setEnabled(prd.isEnabled());
    node->setDisplayName(prd.fullDisplayName());
    node->setAbsoluteFilePathAndLine(FilePath::fromString(prd.location().filePath()).parentDir(), -1);
    node->addNode(std::move(fileNode));

    const QString &productPath = QFileInfo(prd.location().filePath()).absolutePath();
    foreach (const qbs::GroupData &grp, prd.groups()) {
        if (grp.name() == prd.name() && grp.location() == prd.location()) {
            // Set implicit product group right onto this node:
            setupArtifacts(node, grp.allSourceArtifacts());
            continue;
        }
        node->addNode(buildGroupNodeTree(grp, productPath, prd.isEnabled()));
    }

    // Add "Generated Files" Node:
    auto genFiles = std::make_unique<VirtualFolderNode>(FilePath::fromString(prd.buildDirectory()));
    genFiles->setDisplayName(QCoreApplication::translate("QbsProductNode", "Generated files"));
    setupArtifacts(genFiles.get(), prd.generatedArtifacts());
    node->addNode(std::move(genFiles));
}

std::unique_ptr<QbsProductNode> buildProductNodeTree(const qbs::ProductData &prd)
{
    auto result = std::make_unique<QbsProductNode>(prd);

    setupQbsProductData(result.get(), prd);
    return result;
}

void setupProjectNode(QbsProjectNode *node, const qbs::ProjectData &prjData,
                      const qbs::Project &qbsProject)
{
    auto fileNode = std::make_unique<FileNode>(FilePath::fromString(prjData.location().filePath()),
                                               FileType::Project);
    fileNode->setLine(prjData.location().line());

    node->addNode(std::move(fileNode));
    foreach (const qbs::ProjectData &subData, prjData.subProjects()) {
        auto subProject = std::make_unique<QbsProjectNode>(
                            FilePath::fromString(subData.location().filePath()).parentDir());
        setupProjectNode(subProject.get(), subData, qbsProject);
        node->addNode(std::move(subProject));
    }

    foreach (const qbs::ProductData &prd, prjData.products())
        node->addNode(buildProductNodeTree(prd));

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
    QList<QString> referenced = Utils::toList(referencedBuildSystemFiles(p.projectData()));
    Utils::sort(referenced);
    std::set_difference(available.begin(), available.end(), referenced.begin(), referenced.end(),
                        std::back_inserter(result));
    return result;
}

} // namespace

std::unique_ptr<QbsRootProjectNode> QbsNodeTreeBuilder::buildTree(QbsProject *project)
{
    if (!project->qbsProjectData().isValid())
        return {};

    auto root = std::make_unique<QbsRootProjectNode>(project);
    setupProjectNode(root.get(), project->qbsProjectData(), project->qbsProject());

    auto buildSystemFiles = std::make_unique<FolderNode>(project->projectDirectory());
    buildSystemFiles->setDisplayName(QCoreApplication::translate("QbsRootProjectNode", "Qbs files"));

    const FilePath base = project->projectDirectory();
    const QStringList files = unreferencedBuildSystemFiles(project->qbsProject());
    for (const QString &f : files) {
        const FilePath filePath = FilePath::fromString(f);
        if (filePath.isChildOf(base))
            buildSystemFiles->addNestedNode(std::make_unique<FileNode>(filePath, FileType::Project));
    }
    buildSystemFiles->compress();
    root->addNode(std::move(buildSystemFiles));

    return root;
}

} // namespace Internal
} // namespace QbsProjectManager
