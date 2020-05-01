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

#include "mesonprojectnodes.h"
#include <project/mesonbuildconfiguration.h>
#include <project/mesonbuildsystem.h>
#include <mesonpluginconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
namespace MesonProjectManager {
namespace Internal {

MesonProjectNode::MesonProjectNode(const Utils::FilePath &directory)
    : ProjectExplorer::ProjectNode{directory}
{
    static const auto MesonIcon = QIcon(Constants::Icons::MESON);
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(MesonIcon);
    setListInProject(false);
}

MesonFileNode::MesonFileNode(const Utils::FilePath &file)
    : ProjectExplorer::ProjectNode{file}
{
    static const auto MesonFolderIcon = Core::FileIconProvider::directoryIcon(
        Constants::Icons::MESON);
    setIcon(MesonFolderIcon);
    setListInProject(true);
}

MesonTargetNode::MesonTargetNode(const Utils::FilePath &directory, const QString &name)
    : ProjectExplorer::ProjectNode{directory}
    , m_name{name}
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(QIcon(":/projectexplorer/images/build.png"));
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
