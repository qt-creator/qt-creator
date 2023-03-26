// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcuqmlprojectnode.h"

#include <projectexplorer/projectnodes.h>

namespace McuSupport::Internal {

using namespace ProjectExplorer;
using namespace Utils;

McuQmlProjectNode::McuQmlProjectNode(const FilePath &projectFolder, const FilePath &inputsJsonFile)
    : FolderNode(projectFolder)
{
    setDisplayName("QmlProject");
    setIcon(DirectoryIcon(":/projectexplorer/images/fileoverlay_qml.png"));
    setIsGenerated(false);
    setPriority(Node::DefaultProjectPriority);
    setFilePath(projectFolder);
    setListInProject(true);

    const expected_str<QByteArray> expectedJsonContent = inputsJsonFile.fileContents();
    if (!expectedJsonContent)
        return;

    const QJsonDocument inputDoc = QJsonDocument::fromJson(*expectedJsonContent);
    const QVariantMap mainProjectObject = inputDoc.object().toVariantMap();
    const FilePath mainProjectFilePath = FilePath::fromUserInput(
        mainProjectObject.value("qmlProjectFile", "").toString());

    auto mainFileNode = std::make_unique<FileNode>(mainProjectFilePath,
                                                   FileNode::fileTypeForFileName(
                                                       mainProjectFilePath));
    mainFileNode->setPriority(100);
    addNestedNode(std::move(mainFileNode));

    this->populateModuleNode(this, mainProjectObject);

    auto modulesNode = std::make_unique<McuQmlProjectFolderNode>(filePath());
    modulesNode->setDisplayName("QmlProject Modules");
    modulesNode->setIcon(DirectoryIcon(":/projectexplorer/images/fileoverlay_modules.png"));
    modulesNode->setPriority(10);
    for (QVariant moduleVariant : mainProjectObject.value("modulesDependencies", {}).toList()) {
        const QVariantMap moduleObject = moduleVariant.toMap();
        auto moduleNode = std::make_unique<McuQmlProjectFolderNode>(filePath());
        moduleNode->setIcon(DirectoryIcon(":/projectexplorer/images/fileoverlay_qml.png"));
        moduleNode->setDisplayName(
            FilePath::fromUserInput(moduleObject.value("qmlProjectFile", "module").toString())
                .baseName());
        populateModuleNode(moduleNode.get(), moduleObject);
        modulesNode->addNode(std::move(moduleNode));
    }
    addNode(std::move(modulesNode));
}

bool McuQmlProjectNode::populateModuleNode(FolderNode *moduleNode, const QVariantMap &moduleObject)
{
    if (!moduleNode)
        return false;

    const static int NODES_COUNT = 6;
    const static QString nodes[NODES_COUNT] = {"QmlFiles",
                                               "ImageFiles",
                                               "InterfaceFiles",
                                               "FontFiles",
                                               "TranslationFiles",
                                               "ModuleFiles"};
    const static QString icons[NODES_COUNT] = {":/projectexplorer/images/fileoverlay_qml.png",
                                               ":/projectexplorer/images/fileoverlay_qrc.png",
                                               ":/projectexplorer/images/fileoverlay_h.png",
                                               ":/projectexplorer/images/fileoverlay_qrc.png",
                                               ":/projectexplorer/images/fileoverlay_qrc.png",
                                               ":/projectexplorer/images/fileoverlay_qml.png"};
    const static int priorities[NODES_COUNT] = {70, 60, 50, 40, 30, 20};

    for (int i = 0; i < NODES_COUNT; i++) {
        const QString &node = nodes[i];
        const QString &icon = icons[i];
        const int &p = priorities[i];
        auto foldernode = std::make_unique<McuQmlProjectFolderNode>(filePath());
        foldernode->setShowWhenEmpty(false);
        foldernode->setDisplayName(node);
        foldernode->setIcon(DirectoryIcon(icon));
        foldernode->setPriority(p);
        const auto nodeFiles = moduleObject.value(node, {}).toStringList();
        for (auto p : nodeFiles) {
            const FilePath nodePath = FilePath::fromUserInput(p);
            foldernode->addNestedNode(
                std::make_unique<FileNode>(nodePath, FileNode::fileTypeForFileName(nodePath)));
        }
        moduleNode->addNode(std::move(foldernode));
    }
    return true;
}
} // namespace McuSupport::Internal
