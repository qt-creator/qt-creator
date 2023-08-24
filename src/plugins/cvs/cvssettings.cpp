// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cvssettings.h"

#include "cvstr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

using namespace Utils;

namespace Cvs::Internal {

CvsSettings &settings()
{
    static CvsSettings theSettings;
    return theSettings;
}

CvsSettings::CvsSettings()
{
    setAutoApply(false);
    setSettingsGroup("CVS");

    binaryPath.setDefaultValue("cvs" QTC_HOST_EXE_SUFFIX);
    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setHistoryCompleter("Cvs.Command.History");
    binaryPath.setDisplayName(Tr::tr("CVS Command"));
    binaryPath.setLabelText(Tr::tr("CVS command:"));

    cvsRoot.setDisplayStyle(StringAspect::LineEditDisplay);
    cvsRoot.setSettingsKey("Root");
    cvsRoot.setLabelText(Tr::tr("CVS root:"));

    diffOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    diffOptions.setSettingsKey("DiffOptions");
    diffOptions.setDefaultValue("-du");
    diffOptions.setLabelText("Diff options:");

    describeByCommitId.setSettingsKey("DescribeByCommitId");
    describeByCommitId.setDefaultValue(true);
    describeByCommitId.setLabelText(Tr::tr("Describe all files matching commit id"));
    describeByCommitId.setToolTip(Tr::tr("When checked, all files touched by a commit will be "
        "displayed when clicking on a revision number in the annotation view "
        "(retrieved via commit ID). Otherwise, only the respective file will be displayed."));

    diffIgnoreWhiteSpace.setSettingsKey("DiffIgnoreWhiteSpace");

    diffIgnoreBlankLines.setSettingsKey("DiffIgnoreBlankLines");

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    binaryPath, br,
                    cvsRoot
                }
            },
            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Form {
                        timeout, br,
                        diffOptions,
                    },
                    describeByCommitId,
                }
            },
            st
        };
    });

    readSettings();
}

QStringList CvsSettings::addOptions(const QStringList &args) const
{
    const QString cvsRoot = this->cvsRoot();
    if (cvsRoot.isEmpty())
        return args;

    QStringList rc;
    rc.push_back(QLatin1String("-d"));
    rc.push_back(cvsRoot);
    rc.append(args);
    return rc;
}

// CvsSettingsPage

class CvsSettingsPage final : Core::IOptionsPage
{
public:
    CvsSettingsPage()
    {
        setId(VcsBase::Constants::VCS_ID_CVS);
        setDisplayName(Tr::tr("CVS"));
        setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const CvsSettingsPage settingsPage;

} // Cvs::Internal
