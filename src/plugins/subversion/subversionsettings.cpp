/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

    registerAspect(&promptOnSubmit);
    promptOnSubmit.setLabelText(tr("Prompt on submit"));

    QObject::connect(&useAuthentication, &BaseAspect::changed, [this] {
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
                Title(SubversionSettings::tr("Configuration")),
                s.binaryPath
            },

            Group {
                Title(SubversionSettings::tr("Authentication"), &s.useAuthentication),
                Form {
                    s.userName,
                    s.password,
                 }
            },

            Group {
                Title(SubversionSettings::tr("Miscellaneous")),
                Row { s.logCount, s.timeout, Stretch() },
                s.promptOnSubmit,
                s.spaceIgnorantAnnotation,
            },

            Stretch()
        }.attachTo(widget);
    });
}

} // Internal
} // Subversion
