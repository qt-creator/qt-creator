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

#include "qbsnodes.h"
#include "qbsproject.h"
#include "qbssession.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

static FileType fileType(const QJsonObject &artifact)
{
    const QJsonArray fileTags = artifact.value("file-tags").toArray();
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

void setupArtifact(FolderNode *root, const QJsonObject &artifact)
{
    const FilePath path = FilePath::fromString(artifact.value("file-path").toString());
    const FileType type = fileType(artifact);
    const bool isGenerated = artifact.value("is-generated").toBool();

    // A list of human-readable file types that we can reasonably expect
    // to get generated during a build. Extend as needed.
    static const QSet<QString> sourceTags = {
        "c", "cpp", "hpp", "objc", "objcpp", "c_pch_src", "cpp_pch_src", "objc_pch_src",
        "objcpp_pch_src", "asm", "asm_cpp", "linkerscript", "qrc", "java.java"
    };
    auto node = std::make_unique<FileNode>(path, type);
    node->setIsGenerated(isGenerated);
    QSet<QString> fileTags = toSet(transform<QStringList, QJsonArray>(
                                       artifact.value("file-tags").toArray(),
                                       [](const QJsonValue &v) { return v.toString(); }));
    node->setListInProject(!isGenerated || fileTags.intersects(sourceTags));
    root->addNestedNode(std::move(node));
}

static void setupArtifactsForGroup(FolderNode *root, const QJsonObject &group)
{
    forAllArtifacts(group, [root](const QJsonObject &artifact) { setupArtifact(root, artifact); });
    root->compress();
}

static void setupGeneratedArtifacts(FolderNode *root, const QJsonObject &product)
{
    forAllArtifacts(product, ArtifactType::Generated,
                    [root](const QJsonObject &artifact) { setupArtifact(root, artifact); });
    root->compress();
}


static std::unique_ptr<QbsGroupNode> buildGroupNodeTree(const QJsonObject &grp)
{
    const Location location = locationFromObject(grp);
    FilePath baseDir = location.filePath.parentDir();
    QString prefix = grp.value("prefix").toString();
    if (prefix.endsWith('/')) {
        prefix.chop(1);
        if (QFileInfo(prefix).isAbsolute())
            baseDir = FilePath::fromString(prefix);
        else
            baseDir = baseDir.pathAppended(prefix);
    }
    auto result = std::make_unique<QbsGroupNode>(grp);
    result->setAbsoluteFilePathAndLine(baseDir, -1);
    auto fileNode = std::make_unique<FileNode>(FilePath(), FileType::Project);
    fileNode->setAbsoluteFilePathAndLine(location.filePath, location.line);
    result->addNode(std::move(fileNode));
    setupArtifactsForGroup(result.get(), grp);
    return result;
}

static std::unique_ptr<QbsProductNode> buildProductNodeTree(const QJsonObject &prd)
{
    const Location location = locationFromObject(prd);
    auto result = std::make_unique<QbsProductNode>(prd);
    result->setAbsoluteFilePathAndLine(location.filePath.parentDir(), -1);
    auto fileNode = std::make_unique<FileNode>(FilePath(), FileType::Project);
    fileNode->setAbsoluteFilePathAndLine(location.filePath, location.line);
    result->addNode(std::move(fileNode));
    for (const QJsonValue &v : prd.value("groups").toArray()) {
        const QJsonObject grp = v.toObject();
        if (grp.value("name") == prd.value("name")
                && grp.value("location") == prd.value("location")) {
            // Set implicit product group right onto this node:
            setupArtifactsForGroup(result.get(), grp);
            continue;
        }
        result->addNode(buildGroupNodeTree(grp));
    }

    // Add "Generated Files" Node:
    auto genFiles = std::make_unique<VirtualFolderNode>(
                FilePath::fromString(prd.value("build-directory").toString()));
    genFiles->setDisplayName(QCoreApplication::translate("QbsProductNode", "Generated files"));
    setupGeneratedArtifacts(genFiles.get(), prd);
    result->addNode(std::move(genFiles));
    return result;
}

static void setupProjectNode(QbsProjectNode *node)
{
    const Location loc = locationFromObject(node->projectData());
    node->setAbsoluteFilePathAndLine(loc.filePath.parentDir(), -1);
    auto fileNode = std::make_unique<FileNode>(node->filePath(), FileType::Project);
    fileNode->setAbsoluteFilePathAndLine(loc.filePath, loc.line);
    node->addNode(std::move(fileNode));

    for (const QJsonValue &v : node->projectData().value("sub-projects").toArray()) {
        auto subProject = std::make_unique<QbsProjectNode>(v.toObject());
        setupProjectNode(subProject.get());
        node->addNode(std::move(subProject));
    }

    for (const QJsonValue &v : node->projectData().value("products").toArray())
        node->addNode(buildProductNodeTree(v.toObject()));
}

static QSet<QString> referencedBuildSystemFiles(const QJsonObject &prjData)
{
    QSet<QString> result;
    result.insert(prjData.value("location").toObject().value("file-path").toString());
    for (const QJsonValue &v : prjData.value("sub-projects").toArray())
        result.unite(referencedBuildSystemFiles(v.toObject()));
    for (const QJsonValue &v : prjData.value("products").toArray()) {
        const QJsonObject product = v.toObject();
        result.insert(product.value("location").toObject().value("file-path").toString());
        for (const QJsonValue &v : product.value("groups").toArray())
            result.insert(v.toObject().value("location").toObject().value("file-path").toString());
    }
    return result;
}

static QStringList unreferencedBuildSystemFiles(const QJsonObject &project)
{
    QStringList unreferenced = arrayToStringList(project.value("build-system-files"));
    const QList<QString> referenced = toList(referencedBuildSystemFiles(project));
    for (auto it = unreferenced.begin(); it != unreferenced.end(); ) {
        if (referenced.contains(*it))
            it = unreferenced.erase(it);
        else
            ++it;
    }
    return unreferenced;
}

QbsProjectNode *QbsNodeTreeBuilder::buildTree(const QString &projectName,
                                              const Utils::FilePath &projectFile,
                                              const Utils::FilePath &projectDir,
                                              const QJsonObject &projectData)
{
    auto root = std::make_unique<QbsProjectNode>(projectData);

    // If we have no project information at all (i.e. it could not be properly parsed),
    // create the main project file node "manually".
    if (projectData.isEmpty()) {
        auto fileNode = std::make_unique<FileNode>(projectFile, FileType::Project);
        root->addNode(std::move(fileNode));
    } else {
        setupProjectNode(root.get());
    }

    if (root->displayName().isEmpty())
        root->setDisplayName(projectName);
    if (root->displayName().isEmpty())
        root->setDisplayName(projectFile.toFileInfo().completeBaseName());

    auto buildSystemFiles = std::make_unique<FolderNode>(projectDir);
    buildSystemFiles->setDisplayName(QCoreApplication::translate("QbsProjectNode", "Qbs files"));

    const FilePath buildDir = FilePath::fromString(projectData.value("build-directory").toString());
    const QStringList files = unreferencedBuildSystemFiles(projectData);
    for (const QString &f : files) {
        const FilePath filePath = FilePath::fromString(f);
        if (filePath.isChildOf(projectDir)) {
            auto fileNode = std::make_unique<FileNode>(filePath, FileType::Project);
            fileNode->setIsGenerated(filePath.isChildOf(buildDir));
            buildSystemFiles->addNestedNode(std::move(fileNode));
        }
    }
    buildSystemFiles->compress();
    root->addNode(std::move(buildSystemFiles));
    return root.release();
}

} // namespace Internal
} // namespace QbsProjectManager
