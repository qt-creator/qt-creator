// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnprojectnodes.h"

#include "gnbuildconfiguration.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

GNProjectNode::GNProjectNode(const FilePath &directory)
    : ProjectNode(directory)
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setListInProject(false);
}

GNTargetNode::GNTargetNode(const FilePath &directory,
                           const QString &name,
                           const QString &targetType,
                           const QStringList &outputs)
    : ProjectNode(directory)
    , m_name(name)
    , m_targetType(targetType)
    , m_outputs(outputs)
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(":/projectexplorer/images/build.png");
    setListInProject(false);
    setShowWhenEmpty(true);
    if (targetType == "executable") {
        setProductType(ProductType::App);
    } else if (targetType == "shared_library"
               || targetType == "static_library"
               || targetType == "source_set") {
        setProductType(ProductType::Lib);
    } else {
        setProductType(ProductType::Other);
    }
}

void GNTargetNode::build()
{
    if (const auto bc = activeBuildConfig(getProject()))
        static_cast<GNBuildConfiguration *>(bc)->build(m_name);
}

QString GNTargetNode::tooltip() const
{
    return QString("Target: %1\nType: %2\nOutputs: %3").arg(m_name, m_targetType, m_outputs.join(", "));
}

QString GNTargetNode::buildKey() const
{
    return m_name;
}

} // namespace GNProjectManager::Internal
