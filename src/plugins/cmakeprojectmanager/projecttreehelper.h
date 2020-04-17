/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakeprojectnodes.h"

#include <utils/fileutils.h>

#include <memory>

namespace CMakeProjectManager {
namespace Internal {

std::unique_ptr<ProjectExplorer::FolderNode> createCMakeVFolder(const Utils::FilePath &basePath,
                                                                int priority,
                                                                const QString &displayName);

void addCMakeVFolder(ProjectExplorer::FolderNode *base,
                     const Utils::FilePath &basePath,
                     int priority,
                     const QString &displayName,
                     std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&files);

std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&removeKnownNodes(
    const QSet<Utils::FilePath> &knownFiles,
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&files);

void addCMakeInputs(ProjectExplorer::FolderNode *root,
                    const Utils::FilePath &sourceDir,
                    const Utils::FilePath &buildDir,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&sourceInputs,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&buildInputs,
                    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&rootInputs);

QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> addCMakeLists(
    CMakeProjectNode *root, std::vector<std::unique_ptr<ProjectExplorer::FileNode>> &&cmakeLists);

void createProjectNode(const QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
                       const Utils::FilePath &dir,
                       const QString &displayName);
CMakeTargetNode *createTargetNode(
    const QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
    const Utils::FilePath &dir,
    const QString &displayName);

void addHeaderNodes(ProjectExplorer::ProjectNode *root,
                    QSet<Utils::FilePath> &seenHeaders,
                    const QList<const ProjectExplorer::FileNode *> &allFiles);

} // namespace Internal
} // namespace CMakeProjectManager
