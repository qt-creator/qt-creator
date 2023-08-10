// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ninjatoolkitaspect.h"

#include "mesonprojectmanagertr.h"
#include "toolkitaspectwidget.h"

#include <utils/qtcassert.h>

namespace MesonProjectManager::Internal {

static const char TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Ninja";

NinjaToolKitAspectFactory::NinjaToolKitAspectFactory()
{
    setObjectName(QLatin1String("NinjaKitAspect"));
    setId(TOOL_ID);
    setDisplayName(Tr::tr("Ninja Tool"));
    setDescription(Tr::tr("The Ninja tool to use when building a project with Meson.<br>"
                          "This setting is ignored when using other build systems."));
    setPriority(9000);
}

ProjectExplorer::Tasks NinjaToolKitAspectFactory::validate(const ProjectExplorer::Kit *k) const
{
    ProjectExplorer::Tasks tasks;
    const auto tool = NinjaToolKitAspect::ninjaTool(k);
    if (tool && !tool->isValid())
        tasks << ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::Warning,
                                                  Tr::tr("Cannot validate this Ninja executable.")};
    return tasks;
}

void NinjaToolKitAspectFactory::setup(ProjectExplorer::Kit *k)
{
    const auto tool = NinjaToolKitAspect::ninjaTool(k);
    if (!tool) {
        const auto autoDetected = MesonTools::ninjaWrapper();
        if (autoDetected)
            NinjaToolKitAspect::setNinjaTool(k, autoDetected->id());
    }
}

void NinjaToolKitAspectFactory::fix(ProjectExplorer::Kit *k)
{
    setup(k);
}

ProjectExplorer::KitAspectFactory::ItemList NinjaToolKitAspectFactory::toUserOutput(
    const ProjectExplorer::Kit *k) const
{
    const auto tool = NinjaToolKitAspect::ninjaTool(k);
    if (tool)
        return {{Tr::tr("Ninja"), tool->name()}};
    return {{Tr::tr("Ninja"), Tr::tr("Unconfigured")}};
}

ProjectExplorer::KitAspect *NinjaToolKitAspectFactory::createKitAspect(ProjectExplorer::Kit *k) const
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

} // MesonProjectManager::Internal
