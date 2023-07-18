// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mercurialsettings.h"

#include "constants.h"
#include "mercurialtr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Mercurial::Internal {

MercurialSettings &settings()
{
    static MercurialSettings theSettings;
    return theSettings;
}

MercurialSettings::MercurialSettings()
{
    setAutoApply(false);
    setSettingsGroup("Mercurial");

    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(Constants::MERCURIALDEFAULT);
    binaryPath.setDisplayName(Tr::tr("Mercurial Command"));
    binaryPath.setHistoryCompleter("Bazaar.Command.History");
    binaryPath.setLabelText(Tr::tr("Command:"));

    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(Tr::tr("Default username:"));
    userName.setToolTip(Tr::tr("Username to use by default on commit."));

    userEmail.setDisplayStyle(StringAspect::LineEditDisplay);
    userEmail.setLabelText(Tr::tr("Default email:"));
    userEmail.setToolTip(Tr::tr("Email to use by default on commit."));

    diffIgnoreWhiteSpace.setSettingsKey("diffIgnoreWhiteSpace");

    diffIgnoreBlankLines.setSettingsKey("diffIgnoreBlankLines");

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

// MercurialSettingsPage

class MercurialSettingsPage final : public Core::IOptionsPage
{
public:
    MercurialSettingsPage()
    {
        setId(VcsBase::Constants::VCS_ID_MERCURIAL);
        setDisplayName(Tr::tr("Mercurial"));
        setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const MercurialSettingsPage settingsPage;

} // Mercurial::Internal
