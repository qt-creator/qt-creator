// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaarsettings.h"

#include "bazaartr.h"
#include "constants.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Bazaar::Internal {

BazaarSettings &settings()
{
    static BazaarSettings theSettings;
    return theSettings;
}

BazaarSettings::BazaarSettings()
{
    setAutoApply(false);
    setSettingsGroup(Constants::BAZAAR);

    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::BAZAARDEFAULT);
    binaryPath.setDisplayName(Tr::tr("Bazaar Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(Tr::tr("Command:"));

    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");

    logVerbose.setSettingsKey("logVerbose");

    logForward.setSettingsKey("logForward");

    logIncludeMerges.setSettingsKey("logIncludeMerges");

    logFormat.setDisplayStyle(StringAspect::LineEditDisplay);
    logFormat.setSettingsKey("logFormat");
    logFormat.setDefaultValue("long");

    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(Tr::tr("Default username:"));
    userName.setToolTip(Tr::tr("Username to use by default on commit."));

    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(Tr::tr("Default email:"));
    userEmail.setToolTip(Tr::tr("Email to use by default on commit."));

    logCount.setLabelText(Tr::tr("Log count:"));
    logCount.setToolTip(Tr::tr("The number of recent commit logs to show. Choose 0 to see all entries."));

    timeout.setLabelText(Tr::tr("Timeout:"));
    timeout.setSuffix(Tr::tr("s"));

    setLayouter([this] {
        using namespace Layouting;

        return Column {
            Group {
                title(Tr::tr("Configuration")),
                Row { binaryPath }
            },

            Group {
                title(Tr::tr("User")),
                Form {
                    userName, br,
                    userEmail
                }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Row { logCount, timeout, st }
            },
            st
        };
    });

    readSettings();
}

// BazaarSettingsPage

class BazaarSettingsPage final : public Core::IOptionsPage
{
public:
    BazaarSettingsPage()
    {
        setId(VcsBase::Constants::VCS_ID_BAZAAR);
        setDisplayName(Tr::tr("Bazaar"));
        setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const BazaarSettingsPage settingsPage;

} // Bazaar::Internal
