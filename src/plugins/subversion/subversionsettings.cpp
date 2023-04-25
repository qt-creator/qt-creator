// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionsettings.h"

#include "subversiontr.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QSettings>

using namespace Utils;
using namespace VcsBase;

namespace Subversion::Internal {

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
    binaryPath.setDisplayName(Tr::tr("Subversion Command"));
    binaryPath.setLabelText(Tr::tr("Subversion command:"));

    registerAspect(&useAuthentication);
    useAuthentication.setSettingsKey("Authentication");
    useAuthentication.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);

    registerAspect(&userName);
    userName.setSettingsKey("User");
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setLabelText(Tr::tr("Username:"));

    registerAspect(&password);
    password.setSettingsKey("Password");
    password.setDisplayStyle(StringAspect::LineEditDisplay);
    password.setLabelText(Tr::tr("Password:"));

    registerAspect(&spaceIgnorantAnnotation);
    spaceIgnorantAnnotation.setSettingsKey("SpaceIgnorantAnnotation");
    spaceIgnorantAnnotation.setDefaultValue(true);
    spaceIgnorantAnnotation.setLabelText(Tr::tr("Ignore whitespace changes in annotation"));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("DiffIgnoreWhiteSpace");

    registerAspect(&logVerbose);
    logVerbose.setSettingsKey("LogVerbose");

    registerAspect(&logCount);
    logCount.setDefaultValue(1000);
    logCount.setLabelText(Tr::tr("Log count:"));

    registerAspect(&timeout);
    timeout.setLabelText(Tr::tr("Timeout:"));
    timeout.setSuffix(Tr::tr("s"));

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
    setDisplayName(Tr::tr("Subversion"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        SubversionSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Column { s.binaryPath }
            },

            Group {
                title(Tr::tr("Authentication")),
                s.useAuthentication.groupChecker(),
                Form {
                    s.userName,
                    s.password,
                 }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Row { s.logCount, s.timeout, st },
                    s.spaceIgnorantAnnotation,
                }
            },

            st
        }.attachTo(widget);
    });
}

} // Subversion::Internal
