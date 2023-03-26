// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    for (const auto &extraFile : target.extraFiles) {
        root->addNestedNode(
            std::make_unique<ProjectExplorer::FileNode>(Utils::FilePath::fromString(extraFile),
                                                        ProjectExplorer::FileType::Unknown));
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
                    Target::fullName(root->path(), target));
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
    for (const Target &target : targets) {
        buildTargetTree(root, target);
        targetPaths.insert(Utils::FilePath::fromString(target.definedIn).absolutePath());
        addTargetNode(root, target);
    }
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
