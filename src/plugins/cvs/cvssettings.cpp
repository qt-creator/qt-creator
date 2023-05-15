// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cvssettings.h"

#include "cvstr.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Cvs::Internal {

// CvsSettings

CvsSettings::CvsSettings()
{
    setSettingsGroup("CVS");

    registerAspect(&binaryPath);
    binaryPath.setDefaultValue("cvs" QTC_HOST_EXE_SUFFIX);
    binaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setHistoryCompleter(QLatin1String("Cvs.Command.History"));
    binaryPath.setDisplayName(Tr::tr("CVS Command"));
    binaryPath.setLabelText(Tr::tr("CVS command:"));

    registerAspect(&cvsRoot);
    cvsRoot.setDisplayStyle(StringAspect::LineEditDisplay);
    cvsRoot.setSettingsKey("Root");
    cvsRoot.setLabelText(Tr::tr("CVS root:"));

    registerAspect(&diffOptions);
    diffOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    diffOptions.setSettingsKey("DiffOptions");
    diffOptions.setDefaultValue("-du");
    diffOptions.setLabelText("Diff options:");

    registerAspect(&describeByCommitId);
    describeByCommitId.setSettingsKey("DescribeByCommitId");
    describeByCommitId.setDefaultValue(true);
    describeByCommitId.setLabelText(Tr::tr("Describe all files matching commit id"));
    describeByCommitId.setToolTip(Tr::tr("When checked, all files touched by a commit will be "
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

CvsSettingsPage::CvsSettingsPage()
{
    setId(VcsBase::Constants::VCS_ID_CVS);
    setDisplayName(Tr::tr("CVS"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(&settings());

    setLayouter([](QWidget *widget) {
        CvsSettings &s = settings();
        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    s.binaryPath, br,
                    s.cvsRoot
                }
            },
            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Form {
                        s.timeout, br,
                        s.diffOptions,
                    },
                    s.describeByCommitId,
                }
            },
            st
        }.attachTo(widget);
    });
}

CvsSettings &settings()
{
    static CvsSettings theSettings;
    return theSettings;
}

} // Cvs::Internal
