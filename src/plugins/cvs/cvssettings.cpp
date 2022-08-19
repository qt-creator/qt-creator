// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cvssettings.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Cvs {
namespace Internal {

// CvsSettings

CvsSettings::CvsSettings()
{
    setSettingsGroup("CVS");

    registerAspect(&binaryPath);
    binaryPath.setDefaultValue("cvs" QTC_HOST_EXE_SUFFIX);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setHistoryCompleter(QLatin1String("Cvs.Command.History"));
    binaryPath.setDisplayName(tr("CVS Command"));
    binaryPath.setLabelText(tr("CVS command:"));

    registerAspect(&cvsRoot);
    cvsRoot.setDisplayStyle(StringAspect::LineEditDisplay);
    cvsRoot.setSettingsKey("Root");
    cvsRoot.setLabelText(tr("CVS root:"));

    registerAspect(&diffOptions);
    diffOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    diffOptions.setSettingsKey("DiffOptions");
    diffOptions.setDefaultValue("-du");
    diffOptions.setLabelText("Diff options:");

    registerAspect(&describeByCommitId);
    describeByCommitId.setSettingsKey("DescribeByCommitId");
    describeByCommitId.setDefaultValue(true);
    describeByCommitId.setLabelText(tr("Describe all files matching commit id"));
    describeByCommitId.setToolTip(tr("When checked, all files touched by a commit will be "
        "displayed when clicking on a revision number in the annotation view "
        "(retrieved via commit ID). Otherwise, only the respective file will be displayed."));

    registerAspect(&diffIgnoreWhiteSpace);
    diffIgnoreWhiteSpace.setSettingsKey("DiffIgnoreWhiteSpace");

    registerAspect(&diffIgnoreBlankLines);
    diffIgnoreBlankLines.setSettingsKey("DiffIgnoreBlankLines");
}

QStringList CvsSettings::addOptions(const QStringList &args) const
{
    const QString cvsRoot = this->cvsRoot.value();
    if (cvsRoot.isEmpty())
        return args;

    QStringList rc;
    rc.push_back(QLatin1String("-d"));
    rc.push_back(cvsRoot);
    rc.append(args);
    return rc;
}

CvsSettingsPage::CvsSettingsPage(CvsSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_CVS);
    setDisplayName(CvsSettings::tr("CVS"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CvsSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(CvsSettings::tr("Configuration")),
                Form {
                    s.binaryPath,
                    s.cvsRoot
                }
            },
            Group {
                title(CvsSettings::tr("Miscellaneous")),
                Column {
                    Form {
                        s.timeout,
                        s.diffOptions,
                    },
                    s.promptOnSubmit,
                    s.describeByCommitId,
                }
            },
            st
        }.attachTo(widget);
    });
}

} // Internal
} // Cvs
