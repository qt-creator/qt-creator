// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellsettings.h"

#include "haskellconstants.h"
#include "haskelltr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Haskell::Internal {

HaskellSettings &settings()
{
    static HaskellSettings theSettings;
    return theSettings;
}

HaskellSettings::HaskellSettings()
{
    setAutoApply(false);

    stackPath.setSettingsKey("Haskell/StackExecutable");
    stackPath.setExpectedKind(PathChooser::ExistingCommand);
    stackPath.setPromptDialogTitle(Tr::tr("Choose Stack Executable"));
    stackPath.setCommandVersionArguments({"--version"});

    // stack from brew or the installer script from https://docs.haskellstack.org
    // install to /usr/local/bin.
    stackPath.setDefaultValue(HostOsInfo::isAnyUnixHost()
        ? QLatin1String("/usr/local/bin/stack")
        : QLatin1String("stack"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("General")),
                Row { Tr::tr("Stack executable:"), stackPath }
            },
            st,
        };
    });

    readSettings();
}

// HaskellSettingsPage

class HaskellSettingsPage final : public Core::IOptionsPage
{
public:
    HaskellSettingsPage()
    {
        setId(Constants::OPTIONS_GENERAL);
        setDisplayName(Tr::tr("General"));
        setCategory("J.Z.Haskell");
        setDisplayCategory(Tr::tr("Haskell"));
        setCategoryIconPath(":/haskell/images/settingscategory_haskell.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const HaskellSettingsPage settingsPage;

} // Haskell::Internal
