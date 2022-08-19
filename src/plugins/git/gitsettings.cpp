// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "gitsettings.h"

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QDir>

using namespace Utils;
using namespace VcsBase;

namespace Git {
namespace Internal {

GitSettings::GitSettings()
{
    setSettingsGroup("Git");

    path.setDisplayStyle(StringAspect::LineEditDisplay);
    path.setLabelText(tr("Prepend to PATH:"));

    registerAspect(&binaryPath);
    binaryPath.setDefaultValue("git");

    registerAspect(&pullRebase);
    pullRebase.setSettingsKey("PullRebase");
    pullRebase.setLabelText(tr("Pull with rebase"));

    registerAspect(&showTags);
    showTags.setSettingsKey("ShowTags");

    registerAspect(&omitAnnotationDate);
    omitAnnotationDate.setSettingsKey("OmitAnnotationDate");

    registerAspect(&ignoreSpaceChangesInDiff);
    ignoreSpaceChangesInDiff.setSettingsKey("SpaceIgnorantDiff");
    ignoreSpaceChangesInDiff.setDefaultValue(true);

    registerAspect(&ignoreSpaceChangesInBlame);
    ignoreSpaceChangesInBlame.setSettingsKey("SpaceIgnorantBlame");
    ignoreSpaceChangesInBlame.setDefaultValue(true);

    registerAspect(&blameMoveDetection);
    blameMoveDetection.setSettingsKey("BlameDetectMove");
    blameMoveDetection.setDefaultValue(0);

    registerAspect(&diffPatience);
    diffPatience.setSettingsKey("DiffPatience");
    diffPatience.setDefaultValue(true);

    registerAspect(&winSetHomeEnvironment);
    winSetHomeEnvironment.setSettingsKey("WinSetHomeEnvironment");
    winSetHomeEnvironment.setDefaultValue(true);
    winSetHomeEnvironment.setLabelText(tr("Set \"HOME\" environment variable"));
    if (HostOsInfo::isWindowsHost()) {
        const QString currentHome = qtcEnvironmentVariable("HOME");
        const QString toolTip
            = tr("Set the environment variable HOME to \"%1\"\n(%2).\n"
                 "This causes Git to look for the SSH-keys in that location\n"
                 "instead of its installation directory when run outside git bash.")
                  .arg(QDir::homePath(),
                       currentHome.isEmpty() ? tr("not currently set")
                                             : tr("currently set to \"%1\"").arg(currentHome));
        winSetHomeEnvironment.setToolTip(toolTip);
    } else {
        winSetHomeEnvironment.setVisible(false);
    }

    registerAspect(&gitkOptions);
    gitkOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    gitkOptions.setSettingsKey("GitKOptions");
    gitkOptions.setLabelText(tr("Arguments:"));

    registerAspect(&logDiff);
    logDiff.setSettingsKey("LogDiff");
    logDiff.setToolTip(tr("Note that huge amount of commits might take some time."));

    registerAspect(&repositoryBrowserCmd);
    repositoryBrowserCmd.setDisplayStyle(StringAspect::PathChooserDisplay);
    repositoryBrowserCmd.setSettingsKey("RepositoryBrowserCmd");
    repositoryBrowserCmd.setExpectedKind(PathChooser::ExistingCommand);
    repositoryBrowserCmd.setHistoryCompleter("Git.RepoCommand.History");
    repositoryBrowserCmd.setDisplayName(tr("Git Repository Browser Command"));
    repositoryBrowserCmd.setLabelText(tr("Command:"));

    registerAspect(&graphLog);
    graphLog.setSettingsKey("GraphLog");

    registerAspect(&colorLog);
    colorLog.setSettingsKey("ColorLog");
    colorLog.setDefaultValue(true);

    registerAspect(&firstParent);
    firstParent.setSettingsKey("FirstParent");

    registerAspect(&followRenames);
    followRenames.setSettingsKey("FollowRenames");
    followRenames.setDefaultValue(true);

    registerAspect(&lastResetIndex);
    lastResetIndex.setSettingsKey("LastResetIndex");

    registerAspect(&refLogShowDate);
    refLogShowDate.setSettingsKey("RefLogShowDate");

    timeout.setDefaultValue(Utils::HostOsInfo::isWindowsHost() ? 60 : 30);
}

FilePath GitSettings::gitExecutable(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default to pathless binary.
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();

    FilePath binPath = binaryPath.filePath();
    if (!binPath.isAbsolutePath())
        binPath = binPath.searchInPath({path.filePath()}, FilePath::PrependToPath);

    if (binPath.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = tr("The binary \"%1\" could not be located in the path \"%2\"")
                .arg(binaryPath.value(), path.value());
    }
    return binPath;
}

// GitSettingsPage

GitSettingsPage::GitSettingsPage(GitSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_GIT);
    setDisplayName(GitSettings::tr("Git"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        GitSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(GitSettings::tr("Configuration")),
                Column {
                    Row { s.path },
                    s.winSetHomeEnvironment,
                }
            },

            Group {
                title(GitSettings::tr("Miscellaneous")),
                Column {
                    Row { s.logCount, s.timeout, st },
                    s.pullRebase
                }
            },

            Group {
                title(GitSettings::tr("Gitk")),
                Row { s.gitkOptions }
            },

            Group {
                title(GitSettings::tr("Repository Browser")),
                Row { s.repositoryBrowserCmd }
            },

            st
        }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Git
