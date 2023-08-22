// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesontoolkitaspect.h"

#include "mesonprojectmanagertr.h"
#include "toolkitaspectwidget.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace MesonProjectManager::Internal {

const char TOOL_ID[] = "MesonProjectManager.MesonKitInformation.Meson";

void MesonToolKitAspect::setMesonTool(Kit *kit, Utils::Id id)
{
    QTC_ASSERT(kit && id.isValid(), return );
    kit->setValue(TOOL_ID, id.toSetting());
}

Utils::Id MesonToolKitAspect::mesonToolId(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return Utils::Id::fromSetting(kit->value(TOOL_ID));
}

// MesonToolKitAspectFactory

class MesonToolKitAspectFactory final : public KitAspectFactory
{
public:
    MesonToolKitAspectFactory()
    {
        setId(TOOL_ID);
        setDisplayName(Tr::tr("Meson Tool"));
        setDescription(Tr::tr("The Meson tool to use when building a project with Meson.<br>"
                              "This setting is ignored when using other build systems."));
        setPriority(9000);
    }

    Tasks validate(const Kit *k) const final
    {
        Tasks tasks;
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool && !tool->isValid())
            tasks << BuildSystemTask{Task::Warning, Tr::tr("Cannot validate this meson executable.")};
        return tasks;
    }

    void setup(Kit *k) final
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (!tool) {
            const auto autoDetected = MesonTools::mesonWrapper();
            if (autoDetected)
                MesonToolKitAspect::setMesonTool(k, autoDetected->id());
        }
    }
    void fix(Kit *k) final
    {
        setup(k);
    }

    KitAspect *createKitAspect(Kit *k) const
    {
        return new ToolKitAspectWidget{k, this, ToolKitAspectWidget::ToolType::Meson};
    }

    ItemList toUserOutput( const Kit *k) const
    {
        const auto tool = MesonToolKitAspect::mesonTool(k);
        if (tool)
            return {{Tr::tr("Meson"), tool->name()}};
        return {{Tr::tr("Meson"), Tr::tr("Unconfigured")}};
    }
};

const MesonToolKitAspectFactory theMesonKitAspectFactory;

} // MesonProjectManager::Internal
