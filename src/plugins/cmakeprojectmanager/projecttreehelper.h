// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeprojectnodes.h"

#include <utils/filepath.h>

#include <memory>

namespace CMakeProjectManager::Internal {

bool sourcesOrHeadersFolder(const QString &displayName);

std::unique_ptr<ProjectExplorer::FolderNode> createCMakeVFolder(const Utils::FilePath &basePath,
                                                                int priority,
                                                                const QString &displayName);

void addCMakeVFolder(ProjectExplorer::FolderNode *base,
                     const Utils::FilePath &basePath,
                     int priority,
                     const QString &displayName,
                     std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&files,
                     bool listInProject = true);

std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&removeKnownNodes(
    const QSet<Utils::FilePath> &knownFiles,
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&files);

void addCMakeInputs(ProjectExplorer::FolderNode *root,
                    const Utils::FilePath &sourceDir,
                    const Utils::FilePath &buildDir,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&sourceInputs,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&buildInputs,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&rootInputs);

void addCMakePresets(ProjectExplorer::FolderNode *root, const Utils::FilePath &sourceDir);

QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> addCMakeLists(
    CMakeProjectNode *root, std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&cmakeLists);

void createProjectNode(const QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
                       const Utils::FilePath &dir,
                       const QString &displayName);
CMakeTargetNode *createTargetNode(
    const QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
    const Utils::FilePath &dir,
    const QString &displayName);

void addFileSystemNodes(ProjectExplorer::ProjectNode *root,
                        const std::shared_ptr<ProjectExplorer::FolderNode> &folderNode);

} // CMakeProjectManager::Internal
