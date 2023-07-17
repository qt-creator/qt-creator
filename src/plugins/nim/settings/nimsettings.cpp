// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimsettings.h"

#include "../nimconstants.h"
#include "../nimtr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Nim {

NimSettings &settings()
{
    static NimSettings theSettings;
    return theSettings;
}

NimSettings::NimSettings()
{
    setSettingsGroups("Nim", "NimSuggest");
    setAutoApply(false);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title("Nimsuggest"),
                Column { nimSuggestPath }
            },
            st
        };
    });

    nimSuggestPath.setSettingsKey("Command");
    nimSuggestPath.setExpectedKind(PathChooser::ExistingCommand);
    nimSuggestPath.setLabelText(Tr::tr("Path:"));

    readSettings();
}

// NimSettingsPage

class NimSettingsPage final : public Core::IOptionsPage
{
public:
    NimSettingsPage()
    {
        setId(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_ID);
        setDisplayName(Tr::tr("Tools"));
        setCategory(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_CATEGORY);
        setDisplayCategory(Tr::tr("Nim"));
        setCategoryIconPath(":/nim/images/settingscategory_nim.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const NimSettingsPage settingsPage;

} // namespace Nim
