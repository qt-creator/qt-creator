// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "projecttreehelper.h"

#include "cmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager::Internal {

std::unique_ptr<FolderNode> createCMakeVFolder(const Utils::FilePath &basePath,
                                               int priority,
                                               const QString &displayName)
{
    auto newFolder = std::make_unique<VirtualFolderNode>(basePath);
    newFolder->setPriority(priority);
    newFolder->setDisplayName(displayName);
    newFolder->setIsSourcesOrHeaders(displayName == "Source Files"
                                  || displayName == "Header Files");
    return newFolder;
}

void addCMakeVFolder(FolderNode *base,
                     const Utils::FilePath &basePath,
                     int priority,
                     const QString &displayName,
                     std::vector<std::unique_ptr<FileNode>> &&files)
{
    if (files.size() == 0)
        return;
    FolderNode *folder = base;
    if (!displayName.isEmpty()) {
        auto newFolder = createCMakeVFolder(basePath, priority, displayName);
        folder = newFolder.get();
        base->addNode(std::move(newFolder));
    }
    folder->addNestedNodes(std::move(files));
    for (FolderNode *fn : folder->folderNodes())
        fn->compress();
}

std::vector<std::unique_ptr<FileNode>> &&removeKnownNodes(
    const QSet<Utils::FilePath> &knownFiles, std::vector<std::unique_ptr<FileNode>> &&files)
{
    Utils::erase(files, [&knownFiles](const std::unique_ptr<FileNode> &n) {
        return knownFiles.contains(n->filePath());
    });
    return std::move(files);
}

void addCMakeInputs(FolderNode *root,
                    const Utils::FilePath &sourceDir,
                    const Utils::FilePath &buildDir,
                    std::vector<std::unique_ptr<FileNode>> &&sourceInputs,
                    std::vector<std::unique_ptr<FileNode>> &&buildInputs,
                    std::vector<std::unique_ptr<FileNode>> &&rootInputs)
{
    std::unique_ptr<ProjectNode> cmakeVFolder = std::make_unique<CMakeInputsNode>(root->filePath());

    QSet<Utils::FilePath> knownFiles;
    root->forEachGenericNode([&knownFiles](const Node *n) {
        if (n->listInProject())
            knownFiles.insert(n->filePath());
    });

    addCMakeVFolder(cmakeVFolder.get(),
                    sourceDir,
                    1000,
                    QString(),
                    removeKnownNodes(knownFiles, std::move(sourceInputs)));
    addCMakeVFolder(cmakeVFolder.get(),
                    buildDir,
                    100,
                    Tr::tr("<Build Directory>"),
                    removeKnownNodes(knownFiles, std::move(buildInputs)));
    addCMakeVFolder(cmakeVFolder.get(),
                    Utils::FilePath(),
                    10,
                    Tr::tr("<Other Locations>"),
                    removeKnownNodes(knownFiles, std::move(rootInputs)));

    root->addNode(std::move(cmakeVFolder));
}

QHash<Utils::FilePath, ProjectNode *> addCMakeLists(
    CMakeProjectNode *root, std::vector<std::unique_ptr<FileNode>> &&cmakeLists)
{
    QHash<Utils::FilePath, ProjectNode *> cmakeListsNodes;
    cmakeListsNodes.insert(root->filePath(), root);

    const QSet<Utils::FilePath> cmakeDirs
        = Utils::transform<QSet>(cmakeLists, [](const std::unique_ptr<FileNode> &n) {
              return n->filePath().parentDir();
          });
    root->addNestedNodes(std::move(cmakeLists),
                         Utils::FilePath(),
                         [&cmakeDirs, &cmakeListsNodes](const Utils::FilePath &fp)
                             -> std::unique_ptr<ProjectExplorer::FolderNode> {
                             if (cmakeDirs.contains(fp)) {
                                 auto fn = std::make_unique<CMakeListsNode>(fp);
                                 cmakeListsNodes.insert(fp, fn.get());
                                 return fn;
                             }

                             return std::make_unique<FolderNode>(fp);
                         });
    root->compress();
    return cmakeListsNodes;
}

void createProjectNode(const QHash<Utils::FilePath, ProjectNode *> &cmakeListsNodes,
                       const Utils::FilePath &dir,
                       const QString &displayName)
{
    ProjectNode *cmln = cmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, return );

    const Utils::FilePath projectName = dir.pathAppended(".project::" + displayName);

    ProjectNode *pn = cmln->projectNode(projectName);
    if (!pn) {
        auto newNode = std::make_unique<CMakeProjectNode>(projectName);
        pn = newNode.get();
        cmln->addNode(std::move(newNode));
    }
    pn->setDisplayName(displayName);
}

CMakeTargetNode *createTargetNode(const QHash<Utils::FilePath, ProjectNode *> &cmakeListsNodes,
                                  const Utils::FilePath &dir,
                                  const QString &displayName)
{
    ProjectNode *cmln = cmakeListsNodes.value(dir);
    QTC_ASSERT(cmln, return nullptr);

    QString targetId = displayName;

    CMakeTargetNode *tn = static_cast<CMakeTargetNode *>(
        cmln->findNode([&targetId](const Node *n) { return n->buildKey() == targetId; }));
    if (!tn) {
        auto newNode = std::make_unique<CMakeTargetNode>(dir, displayName);
        tn = newNode.get();
        cmln->addNode(std::move(newNode));
    }
    tn->setDisplayName(displayName);
    return tn;
}

template<typename Result>
static std::unique_ptr<Result> cloneFolderNode(FolderNode *node)
{
    auto folderNode = std::make_unique<Result>(node->filePath());
    folderNode->setDisplayName(node->displayName());
    for (Node *node : node->nodes()) {
        if (FileNode *fn = node->asFileNode()) {
            folderNode->addNode(std::unique_ptr<FileNode>(fn->clone()));
        } else if (FolderNode *fn = node->asFolderNode()) {
            folderNode->addNode(cloneFolderNode<FolderNode>(fn));
        } else {
            QTC_CHECK(false);
        }
    }
    return folderNode;
}

void addFileSystemNodes(ProjectNode *root, const std::shared_ptr<FolderNode> &folderNode)
{
    QTC_ASSERT(root, return );

    auto fileSystemNode = cloneFolderNode<VirtualFolderNode>(folderNode.get());
    // just before special nodes like "CMake Modules"
    fileSystemNode->setPriority(Node::DefaultPriority - 6);
    fileSystemNode->setDisplayName(Tr::tr("<File System>"));
    fileSystemNode->setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN));

    if (!fileSystemNode->isEmpty()) {
        // make file system nodes less probable to be selected when syncing with the current document
        fileSystemNode->forEachGenericNode([](Node *n) {
            n->setPriority(n->priority() + Node::DefaultProjectFilePriority + 1);
            n->setEnabled(false);
        });
        root->addNode(std::move(fileSystemNode));
    }
}

} // CMakeProjectManager::Internal
