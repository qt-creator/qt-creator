// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonprojectnodes.h"

#include "mesonbuildconfiguration.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

MesonProjectNode::MesonProjectNode(const FilePath &directory)
    : ProjectNode(directory)
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(Constants::Icons::MESON);
    setListInProject(false);
}

MesonFileNode::MesonFileNode(const FilePath &file)
    : ProjectNode(file)
{
    setIcon(DirectoryIcon(Constants::Icons::MESON));
    setListInProject(true);
}

MesonTargetNode::MesonTargetNode(const FilePath &directory, const QString &name)
    : ProjectNode(directory)
    , m_name(name)
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(":/projectexplorer/images/build.png");
    setListInProject(false);
    setShowWhenEmpty(true);
    setProductType(ProductType::Other);
}

void MesonTargetNode::build()
{
    Project *p = getProject();
    ProjectExplorer::Target *t = p ? p->activeTarget() : nullptr;
    if (t)
        static_cast<MesonBuildConfiguration *>(t->buildSystem()->buildConfiguration())->build(m_name);
}

QString MesonTargetNode::tooltip() const
{
    return {};
}

QString MesonTargetNode::buildKey() const
{
    return m_name;
}

} // MesonProjectManager::Internal
