// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesontoolkitaspect.h"

#include "mesonprojectmanagertr.h"
#include "toolkitaspectwidget.h"

#include <utils/qtcassert.h>

namespace MesonProjectManager {
namespace Internal {

static const char TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Meson";

MesonToolKitAspect::MesonToolKitAspect()
{
    setObjectName(QLatin1String("MesonKitAspect"));
    setId(TOOL_ID);
    setDisplayName(Tr::tr("Meson Tool"));
    setDescription(Tr::tr("The Meson tool to use when building a project with Meson.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(9000);
}

ProjectExplorer::Tasks MesonToolKitAspect::validate(const ProjectExplorer::Kit *k) const
{
    ProjectExplorer::Tasks tasks;
    const auto tool = mesonTool(k);
    if (tool && !tool->isValid())
        tasks << ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::Warning,
                                                  Tr::tr("Cannot validate this meson executable.")};
    return tasks;
}

void MesonToolKitAspect::setup(ProjectExplorer::Kit *k)
{
    const auto tool = mesonTool(k);
    if (!tool) {
        const auto autoDetected = MesonTools::mesonWrapper();
        if (autoDetected)
            setMesonTool(k, autoDetected->id());
    }
}

void MesonToolKitAspect::fix(ProjectExplorer::Kit *k)
{
    setup(k);
}

ProjectExplorer::KitAspect::ItemList MesonToolKitAspect::toUserOutput(
    const ProjectExplorer::Kit *k) const
{
    const auto tool = mesonTool(k);
    if (tool)
        return {{Tr::tr("Meson"), tool->name()}};
    return {{Tr::tr("Meson"), Tr::tr("Unconfigured")}};
}

ProjectExplorer::KitAspectWidget *MesonToolKitAspect::createConfigWidget(ProjectExplorer::Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new ToolKitAspectWidget{k, this, ToolKitAspectWidget::ToolType::Meson};
}

void MesonToolKitAspect::setMesonTool(ProjectExplorer::Kit *kit, Utils::Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(TOOL_ID, id.toSetting());
}

Utils::Id MesonToolKitAspect::mesonToolId(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Utils::Id::fromSetting(kit->value(TOOL_ID));
}
} // namespace Internal
} // namespace MesonProjectManager
