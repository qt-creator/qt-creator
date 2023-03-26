// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonprojectnodes.h"

#include "mesonbuildconfiguration.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

namespace MesonProjectManager {
namespace Internal {

MesonProjectNode::MesonProjectNode(const Utils::FilePath &directory)
    : ProjectExplorer::ProjectNode{directory}
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(Constants::Icons::MESON);
    setListInProject(false);
}

MesonFileNode::MesonFileNode(const Utils::FilePath &file)
    : ProjectExplorer::ProjectNode{file}
{
    setIcon(ProjectExplorer::DirectoryIcon(Constants::Icons::MESON));
    setListInProject(true);
}

MesonTargetNode::MesonTargetNode(const Utils::FilePath &directory, const QString &name)
    : ProjectExplorer::ProjectNode{directory}
    , m_name{name}
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(":/projectexplorer/images/build.png");
    setListInProject(false);
    setShowWhenEmpty(true);
    setProductType(ProjectExplorer::ProductType::Other);
}

void MesonTargetNode::build()
{
    ProjectExplorer::Project *p = getProject();
    ProjectExplorer::Target *t = p ? p->activeTarget() : nullptr;
    if (t)
        static_cast<MesonBuildSystem *>(t->buildSystem())->mesonBuildConfiguration()->build(m_name);
}

QString MesonTargetNode::tooltip() const
{
    return {};
}

QString MesonTargetNode::buildKey() const
{
    return m_name;
}

} // namespace Internal
} // namespace MesonProjectManager
