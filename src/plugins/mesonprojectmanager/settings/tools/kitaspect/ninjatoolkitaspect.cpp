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

#include "ninjatoolkitaspect.h"
#include "toolkitaspectwidget.h"
#include "utils/qtcassert.h"

namespace MesonProjectManager {
namespace Internal {

static const char TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Ninja";

NinjaToolKitAspect::NinjaToolKitAspect()
{
    setObjectName(QLatin1String("NinjaKitAspect"));
    setId(TOOL_ID);
    setDisplayName(tr("Ninja Tool"));
    setDescription(tr("The Ninja tool to use when building a project with Meson.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(9000);
}

ProjectExplorer::Tasks NinjaToolKitAspect::validate(const ProjectExplorer::Kit *k) const
{
    ProjectExplorer::Tasks tasks;
    const auto tool = ninjaTool(k);
    if (tool && !tool->isValid())
        tasks << ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::Warning,
                                                  tr("Cannot validate this Ninja executable.")};
    return tasks;
}

void NinjaToolKitAspect::setup(ProjectExplorer::Kit *k)
{
    const auto tool = ninjaTool(k);
    if (!tool) {
        const auto autoDetected = MesonTools::ninjaWrapper();
        if (autoDetected)
            setNinjaTool(k, autoDetected->id());
    }
}

void NinjaToolKitAspect::fix(ProjectExplorer::Kit *k)
{
    setup(k);
}

ProjectExplorer::KitAspect::ItemList NinjaToolKitAspect::toUserOutput(
    const ProjectExplorer::Kit *k) const
{
    const auto tool = ninjaTool(k);
    if (tool)
        return {{tr("Ninja"), tool->name()}};
    return {{tr("Ninja"), tr("Unconfigured")}};
}

ProjectExplorer::KitAspectWidget *NinjaToolKitAspect::createConfigWidget(ProjectExplorer::Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new ToolKitAspectWidget{k, this, ToolKitAspectWidget::ToolType::Ninja};
}

void NinjaToolKitAspect::setNinjaTool(ProjectExplorer::Kit *kit, Utils::Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(TOOL_ID, id.toSetting());
}

Utils::Id NinjaToolKitAspect::ninjaToolId(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Utils::Id::fromSetting(kit->value(TOOL_ID));
}
} // namespace Internal
} // namespace MesonProjectManager
