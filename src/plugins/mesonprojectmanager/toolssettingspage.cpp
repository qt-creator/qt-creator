// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingspage.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "toolssettingswidget.h"

namespace MesonProjectManager {
namespace Internal {

ToolsSettingsPage::ToolsSettingsPage()
{
    setId(Constants::SettingsPage::TOOLS_ID);
    setDisplayName(Tr::tr("Tools"));
    setCategory(Constants::SettingsPage::CATEGORY);
    setWidgetCreator([]() { return new ToolsSettingsWidget; });
}

} // namespace Internal
} // namespace MesonProjectManager
