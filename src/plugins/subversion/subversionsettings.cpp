// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionsettings.h"

#include "subversionclient.h"
#include "subversionplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QSettings>

using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

// SubversionSettings

SubversionSettings::SubversionSettings()
{
    setAutoApply(false);
    setSettingsGroup("Subversion");

    registerAspect(&binaryPath);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setHistoryCompleter("Subversion.Command.History");
    binaryPath.setDefaultValue("svn" QTC_HOST_EXE_SUFFIX);
    binaryPath.setDisplayName(tr("Subversion Command"));
    binaryPath.setLabelText(tr("Subversion command:"));

    registerAspect(&useAuthentication);
    useAuthentication.setSettingsKey("Authentication");
    useAuthentication.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);

    registerAspect(&userName);
    userName.setSettingsKey("User");
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(tr("Username:"));

    registerAspect(&password);
    password.setSettingsKey("Password");
    password.setDisplayStyle(StringAspect::LineEditDisplay);
    password.setLabelText(tr("Password:"));

    registerAspect(&spaceIgnorantAnnotation);
    spaceIgnorantAnnotation.setSettingsKey("SpaceIgnorantAnnotation");
    spaceIgnorantAnnotation.setDefaultValue(true);
    spaceIgnorantAnnotation.setLabelText(tr("Ignore whitespace changes in annotation"));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("DiffIgnoreWhiteSpace");

    registerAspect(&logVerbose);
    logVerbose.setSettingsKey("LogVerbose");

    registerAspect(&logCount);
    logCount.setDefaultValue(1000);
    logCount.setLabelText(tr("Log count:"));

    registerAspect(&timeout);
    timeout.setLabelText(tr("Timeout:"));
    timeout.setSuffix(tr("s"));

    QObject::connect(&useAuthentication, &BaseAspect::changed, this, [this] {
        userName.setEnabled(useAuthentication.value());
        password.setEnabled(useAuthentication.value());
    });
}

bool SubversionSettings::hasAuthentication() const
{
    return useAuthentication.value() && !userName.value().isEmpty();
}

SubversionSettingsPage::SubversionSettingsPage(SubversionSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_SUBVERSION);
    setDisplayName(SubversionSettings::tr("Subversion"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        SubversionSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(SubversionSettings::tr("Configuration")),
                Column { s.binaryPath }
            },

            Group {
                title(SubversionSettings::tr("Authentication"), &s.useAuthentication),
                Form {
                    s.userName,
                    s.password,
                 }
            },

            Group {
                title(SubversionSettings::tr("Miscellaneous")),
                Column {
                    Row { s.logCount, s.timeout, st },
                    s.spaceIgnorantAnnotation,
                }
            },

            st
        }.attachTo(widget);
    });
}

} // Internal
} // Subversion
