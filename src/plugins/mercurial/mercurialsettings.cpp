// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mercurialsettings.h"

#include "constants.h"
#include "mercurialtr.h"

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Mercurial::Internal {

MercurialSettings::MercurialSettings()
{
    setSettingsGroup("Mercurial");
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::MERCURIALDEFAULT);
    binaryPath.setDisplayName(Tr::tr("Mercurial Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(Tr::tr("Command:"));

    registerAspect(&userName);
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(Tr::tr("Default username:"));
    userName.setToolTip(Tr::tr("Username to use by default on commit."));

    registerAspect(&userEmail);
    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(Tr::tr("Default email:"));
    userEmail.setToolTip(Tr::tr("Email to use by default on commit."));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    registerAspect(&diffIgnoreBlankLines);
    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");
}

// MercurialSettingsPage

MercurialSettingsPage::MercurialSettingsPage(MercurialSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(Tr::tr("Mercurial"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        MercurialSettings &s = *settings;
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

} // Mercurial::Internal
