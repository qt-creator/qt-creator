// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bazaarsettings.h"

#include "bazaarclient.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Bazaar {
namespace Internal {

BazaarSettings::BazaarSettings()
{
    setSettingsGroup(Constants::BAZAAR);
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::BAZAARDEFAULT);
    binaryPath.setDisplayName(tr("Bazaar Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(tr("Command:"));

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
    userName.setLabelText(tr("Default username:"));
    userName.setToolTip(tr("Username to use by default on commit."));

    registerAspect(&userEmail);
    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(tr("Default email:"));
    userEmail.setToolTip(tr("Email to use by default on commit."));

    registerAspect(&logCount);
    logCount.setLabelText(tr("Log count:"));
    logCount.setToolTip(tr("The number of recent commit logs to show. Choose 0 to see all entries."));

    registerAspect(&logCount);
    timeout.setLabelText(tr("Timeout:"));
    timeout.setSuffix(tr("s"));
}

// BazaarSettingsPage

BazaarSettingsPage::BazaarSettingsPage(BazaarSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(BazaarSettings::tr("Bazaar"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        BazaarSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(BazaarSettings::tr("Configuration")),
                Row { s.binaryPath }
            },

            Group {
                title(BazaarSettings::tr("User")),
                Form {
                    s.userName,
                    s.userEmail
                }
            },

            Group {
                title(BazaarSettings::tr("Miscellaneous")),
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

} // Internal
} // Bazaar
