/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                Title(BazaarSettings::tr("Configuration")),
                Row { s.binaryPath }
            },

            Group {
                Title(BazaarSettings::tr("User")),
                Form {
                    s.userName,
                    s.userEmail
                }
            },

            Group {
                Title(BazaarSettings::tr("Miscellaneous")),
                Row {
                    s.logCount,
                    s.timeout,
                    Stretch()
                }
            },
            Stretch()

        }.attachTo(widget);
    });
}

} // Internal
} // Bazaar
