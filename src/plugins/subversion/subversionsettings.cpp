// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionsettings.h"

#include "subversiontr.h"

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;
using namespace VcsBase;

namespace Subversion::Internal {

static SubversionSettings *theSettings;

SubversionSettings &settings()
{
    return *theSettings;
}

SubversionSettings::SubversionSettings()
{
    theSettings = this;

    setId(VcsBase::Constants::VCS_ID_SUBVERSION);
    setDisplayName(Tr::tr("Subversion"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettingsGroup("Subversion");

    registerAspect(&binaryPath);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setHistoryCompleter("Subversion.Command.History");
    binaryPath.setDefaultValue("svn" QTC_HOST_EXE_SUFFIX);
    binaryPath.setDisplayName(Tr::tr("Subversion Command"));
    binaryPath.setLabelText(Tr::tr("Subversion command:"));

    registerAspect(&useAuthentication);
    useAuthentication.setSettingsKey("Authentication");
    useAuthentication.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

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
        userName.setEnabled(useAuthentication());
        password.setEnabled(useAuthentication());
    });

    setLayouter([this](QWidget *widget) {
        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Column { binaryPath }
            },

            Group {
                title(Tr::tr("Authentication")),
                useAuthentication.groupChecker(),
                Form {
                    userName, br,
                    password,
                 }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Row { logCount, timeout, st },
                    spaceIgnorantAnnotation,
                }
            },

            st
        }.attachTo(widget);
    });
}

bool SubversionSettings::hasAuthentication() const
{
    return useAuthentication() && !userName().isEmpty();
}

} // Subversion::Internal
