// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaarsettings.h"

#include "bazaartr.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Bazaar::Internal {

BazaarSettings::BazaarSettings()
{
    setSettingsGroup(Constants::BAZAAR);
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::BAZAARDEFAULT);
    binaryPath.setDisplayName(Tr::tr("Bazaar Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(Tr::tr("Command:"));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    registerAspect(&diffIgnoreBlankLines);
    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");

    registerAspect(&logVerbose);
    logVerbose.setSettingsKey("logVerbose");

    registerAspect(&logFormat);
    logForward.setSettingsKey("logForward");

    registerAspect(&logIncludeMerges);
    logIncludeMerges.setSettingsKey("logIncludeMerges");

    registerAspect(&logFormat);
    logFormat.setDisplayStyle(StringAspect::LineEditDisplay);
    logFormat.setSettingsKey("logFormat");
    logFormat.setDefaultValue("long");

    registerAspect(&userName);
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(Tr::tr("Default username:"));
    userName.setToolTip(Tr::tr("Username to use by default on commit."));

    registerAspect(&userEmail);
    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(Tr::tr("Default email:"));
    userEmail.setToolTip(Tr::tr("Email to use by default on commit."));

    registerAspect(&logCount);
    logCount.setLabelText(Tr::tr("Log count:"));
    logCount.setToolTip(Tr::tr("The number of recent commit logs to show. Choose 0 to see all entries."));

    registerAspect(&logCount);
    timeout.setLabelText(Tr::tr("Timeout:"));
    timeout.setSuffix(Tr::tr("s"));
}

// BazaarSettingsPage

BazaarSettingsPage::BazaarSettingsPage(BazaarSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(Tr::tr("Bazaar"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        BazaarSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Row { s.binaryPath }
            },

            Group {
                title(Tr::tr("User")),
                Form {
                    s.userName,
                    s.userEmail
                }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Row {
                    s.logCount,
                    s.timeout,
                    st
                }
            },
            st
        }.attachTo(widget);
    });
}

} // Bazaar::Internal
