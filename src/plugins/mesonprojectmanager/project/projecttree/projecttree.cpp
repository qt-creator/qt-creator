/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "projecttree.h"
#include <set>
namespace MesonProjectManager {
namespace Internal {
ProjectTree::ProjectTree() {}

void buildTargetTree(std::unique_ptr<MesonProjectNode> &root, const Target &target)
{
    const auto path = Utils::FilePath::fromString(target.definedIn);
    for (const auto &group : target.sources) {
        for (const auto &file : group.sources) {
            root->addNestedNode(
                std::make_unique<ProjectExplorer::FileNode>(Utils::FilePath::fromString(file),
                                                            ProjectExplorer::FileType::Source));
        }
    }
}

void addTargetNode(std::unique_ptr<MesonProjectNode> &root, const Target &target)
{
    root->findNode([&root, &target, path = Utils::FilePath::fromString(target.definedIn)](
                       ProjectExplorer::Node *node) {
        if (node->filePath() == path.absolutePath()) {
            auto asFolder = dynamic_cast<ProjectExplorer::FolderNode *>(node);
            if (asFolder) {
                auto targetNode = std::make_unique<MesonTargetNode>(
                    path.absolutePath().pathAppended(target.name),
                    Target::fullName(Utils::FilePath::fromString(root->path()), target));
                targetNode->setDisplayName(target.name);
                asFolder->addNode(std::move(targetNode));
            }
            return true;
        }
        return false;
    });
}

void addOptionsFile(std::unique_ptr<MesonProjectNode> &project)
{
    auto meson_options = project->filePath().pathAppended("meson_options.txt");
    if (meson_options.exists())
        project->addNestedNode(
            std::make_unique<ProjectExplorer::FileNode>(meson_options,
                                                        ProjectExplorer::FileType::Project));
}

std::unique_ptr<MesonProjectNode> ProjectTree::buildTree(const Utils::FilePath &srcDir,
                                                         const TargetsList &targets,
                                                         const std::vector<Utils::FilePath> &bsFiles)
{
    using namespace ProjectExplorer;
    std::set<Utils::FilePath> targetPaths;
    auto root = std::make_unique<MesonProjectNode>(srcDir);
    std::for_each(std::cbegin(targets),
                  std::cend(targets),
                  [&root, &targetPaths](const Target &target) {
                      buildTargetTree(root, target);
                      targetPaths.insert(
                          Utils::FilePath::fromString(target.definedIn).absolutePath());
                      addTargetNode(root, target);
                  });
    for (Utils::FilePath bsFile : bsFiles) {
        if (!bsFile.toFileInfo().isAbsolute())
            bsFile = srcDir.pathAppended(bsFile.toString());
        root->addNestedNode(
            std::make_unique<ProjectExplorer::FileNode>(bsFile, ProjectExplorer::FileType::Project));
    }
    return root;
}
} // namespace Internal
} // namespace MesonProjectManager
