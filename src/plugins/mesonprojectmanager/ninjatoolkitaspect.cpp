// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ninjatoolkitaspect.h"

#include "mesonprojectmanagertr.h"
#include "toolkitaspectwidget.h"

#include <utils/qtcassert.h>

namespace MesonProjectManager {
namespace Internal {

static const char TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Ninja";

NinjaToolKitAspect::NinjaToolKitAspect()
{
    setObjectName(QLatin1String("NinjaKitAspect"));
    setId(TOOL_ID);
    setDisplayName(Tr::tr("Ninja Tool"));
    setDescription(Tr::tr("The Ninja tool to use when building a project with Meson.<br>"
                          "This setting is ignored when using other build systems."));
    setPriority(9000);
}

ProjectExplorer::Tasks NinjaToolKitAspect::validate(const ProjectExplorer::Kit *k) const
{
    ProjectExplorer::Tasks tasks;
    const auto tool = ninjaTool(k);
    if (tool && !tool->isValid())
        tasks << ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::Warning,
                                                  Tr::tr("Cannot validate this Ninja executable.")};
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
        return {{Tr::tr("Ninja"), tool->name()}};
    return {{Tr::tr("Ninja"), Tr::tr("Unconfigured")}};
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
