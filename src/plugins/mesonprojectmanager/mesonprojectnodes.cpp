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

MesonTargetNode::MesonTargetNode(const FilePath &directory, const QString &name,const QString &target_type, const QStringList &filenames, bool build_by_default)
    : ProjectNode(directory)
    , m_name(name)
    , m_target_type(target_type)
    , m_filenames(filenames)
    , m_build_by_default(build_by_default)
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
    if (const auto bc = p ? p->activeBuildConfiguration() : nullptr)
        static_cast<MesonBuildConfiguration *>(bc)->build(m_name);
}

QString MesonTargetNode::tooltip() const
{
    return QString(R"(Target: %1
Type: %2
Build by default: %3
Produced files: %4)").arg(m_name, m_target_type,
             m_build_by_default ? QStringLiteral("Yes"):QStringLiteral("No"),
             m_filenames.join(", "));
}

QString MesonTargetNode::buildKey() const
{
    return m_name;
}

} // MesonProjectManager::Internal
