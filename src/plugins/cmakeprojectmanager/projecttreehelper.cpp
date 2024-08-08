// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projecttreehelper.h"

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

bool defaultCMakeSourceGroupFolder(const QString &displayName)
{
    return displayName == "Source Files" || displayName == "Header Files"
           || displayName == "Resources" || displayName == ""
           || displayName == "Precompile Header File" || displayName == "CMake Rules"
           || displayName == "Object Files" || displayName == "Forms"
           || displayName == "State charts";
}

static QIcon iconForSourceGroup(const QString &sourceGroup)
{
    static const QHash<QString, QString> sourceGroupToOverlay = {
        {"Forms", ProjectExplorer::Constants::FILEOVERLAY_UI},
        {"Header Files", ProjectExplorer::Constants::FILEOVERLAY_H},
        {"Resources", ProjectExplorer::Constants::FILEOVERLAY_QRC},
        {"State charts", ProjectExplorer::Constants::FILEOVERLAY_SCXML},
        {"Source Files", ProjectExplorer::Constants::FILEOVERLAY_CPP},
    };

    return sourceGroupToOverlay.contains(sourceGroup)
               ? FileIconProvider::directoryIcon(sourceGroupToOverlay.value(sourceGroup))
               : FileIconProvider::icon(QFileIconProvider::Folder);
}

std::unique_ptr<FolderNode> createCMakeVFolder(const Utils::FilePath &basePath,
                                               int priority,
                                               const QString &displayName)
{
    auto newFolder = std::make_unique<VirtualFolderNode>(basePath);
    newFolder->setPriority(priority);
    newFolder->setDisplayName(displayName);
    newFolder->setIcon([displayName] { return iconForSourceGroup(displayName); });
    newFolder->setIsSourcesOrHeaders(defaultCMakeSourceGroupFolder(displayName));
    return newFolder;
}

void addCMakeVFolder(FolderNode *base,
                     const Utils::FilePath &basePath,
                     int priority,
                     const QString &displayName,
                     std::vector<std::unique_ptr<FileNode>> &&files,
                     bool listInProject)
{
    if (files.size() == 0)
        return;
    FolderNode *folder = base;
    if (!displayName.isEmpty()) {
        auto newFolder = createCMakeVFolder(basePath, priority, displayName);
        folder = newFolder.get();
        base->addNode(std::move(newFolder));
    }
    if (!listInProject) {
        for (auto it = files.begin(); it != files.end(); ++it)
            (*it)->setListInProject(false);
    }
    folder->addNestedNodes(std::move(files));
    folder->forEachFolderNode([] (FolderNode *fn) { fn->compress(); });
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
    root->forEachGenericNode([&knownFiles](const Node *n) { knownFiles.insert(n->filePath()); });

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
                    removeKnownNodes(knownFiles, std::move(rootInputs)),
                    /*listInProject=*/false);

    root->addNode(std::move(cmakeVFolder));
}

void addCMakePresets(FolderNode *root, const Utils::FilePath &sourceDir)
{
    QStringList presetFileNames;
    presetFileNames << "CMakePresets.json";
    presetFileNames << "CMakeUserPresets.json";

    const CMakeProject *cp = static_cast<const CMakeProject *>(
        ProjectManager::projectForFile(sourceDir.pathAppended(Constants::CMAKE_LISTS_TXT)));

    if (cp && cp->presetsData().include)
        presetFileNames.append(cp->presetsData().include.value());

    std::vector<std::unique_ptr<FileNode>> presets;
    for (const auto &fileName : presetFileNames) {
        Utils::FilePath file = sourceDir.pathAppended(fileName);
        if (file.exists())
            presets.push_back(std::make_unique<FileNode>(file, Node::fileTypeForFileName(file)));
    }

    if (presets.empty())
        return;

    std::unique_ptr<ProjectNode> cmakeVFolder = std::make_unique<CMakePresetsNode>(root->filePath());
    addCMakeVFolder(cmakeVFolder.get(), sourceDir, 1000, QString(), std::move(presets));

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
