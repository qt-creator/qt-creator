// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mercurialsettings.h"

#include "constants.h"

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Mercurial {
namespace Internal {

MercurialSettings::MercurialSettings()
{
    setSettingsGroup("Mercurial");
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::MERCURIALDEFAULT);
    binaryPath.setDisplayName(tr("Mercurial Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(tr("Command:"));

    registerAspect(&userName);
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(tr("Default username:"));
    userName.setToolTip(tr("Username to use by default on commit."));

    registerAspect(&userEmail);
    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(tr("Default email:"));
    userEmail.setToolTip(tr("Email to use by default on commit."));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    registerAspect(&diffIgnoreBlankLines);
    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");
}

// MercurialSettingsPage

MercurialSettingsPage::MercurialSettingsPage(MercurialSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(MercurialSettings::tr("Mercurial"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        MercurialSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(MercurialSettings::tr("Configuration")),
                Row { s.binaryPath }
            },

            Group {
                title(MercurialSettings::tr("User")),
                Form {
                    s.userName,
                    s.userEmail
                }
            },

            Group {
                title(MercurialSettings::tr("Miscellaneous")),
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

} // namespace Internal
} // namespace Mercurial
