// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designersettings.h"

#include "designerconstants.h"
#include "designertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Designer::Internal {

DesignerSettings::DesignerSettings()
{
    setSettingsGroup("Designer");
    setAutoApply(false);

    generatePointerToMemberConnections.setSettingsKey("GeneratePointerToMemberConnections");
    generatePointerToMemberConnections.setDefaultValue(true);
    generatePointerToMemberConnections.setLabelText(
        Tr::tr("Generate pointer-to-member connections in \"Go to Slot\""));
    generatePointerToMemberConnections.setToolTip(
        Tr::tr("Inserts an explicit connect() statement using the function pointer syntax "
               "instead of relying on QMetaObject::connectSlotsByName() and an on_...() slot "
               "name."));

    setLayouter([this] {
        using namespace Layouting;
        return Column { generatePointerToMemberConnections, st };
    });

    readSettings();
}

DesignerSettings &designerSettings()
{
    static DesignerSettings theSettings;
    return theSettings;
}

class DesignerSettingsPage final : public Core::IOptionsPage
{
public:
    DesignerSettingsPage()
    {
        setId("Designer.CreatorIntegration");
        setDisplayName(Tr::tr("Qt Widgets Designer"));
        setCategory(Designer::Constants::SETTINGS_CATEGORY);
        setSettingsProvider([] { return &designerSettings(); });
    }
};

void setupDesignerSettingsPage()
{
    static DesignerSettingsPage theDesignerSettingsPage;
}

} // namespace Designer::Internal
