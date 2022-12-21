// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsettings.h"

#include "gittr.h"

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QDir>

using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

GitSettings::GitSettings()
{
    setSettingsGroup("Git");

    path.setDisplayStyle(StringAspect::LineEditDisplay);
    path.setLabelText(Tr::tr("Prepend to PATH:"));

    registerAspect(&binaryPath);
    binaryPath.setDefaultValue("git");

    registerAspect(&pullRebase);
    pullRebase.setSettingsKey("PullRebase");
    pullRebase.setLabelText(Tr::tr("Pull with rebase"));

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
    winSetHomeEnvironment.setLabelText(Tr::tr("Set \"HOME\" environment variable"));
    if (HostOsInfo::isWindowsHost()) {
        const QString currentHome = qtcEnvironmentVariable("HOME");
        const QString toolTip
            = Tr::tr("Set the environment variable HOME to \"%1\"\n(%2).\n"
                 "This causes Git to look for the SSH-keys in that location\n"
                 "instead of its installation directory when run outside git bash.")
                  .arg(QDir::homePath(),
                       currentHome.isEmpty() ? Tr::tr("not currently set")
                                             : Tr::tr("currently set to \"%1\"").arg(currentHome));
        winSetHomeEnvironment.setToolTip(toolTip);
    } else {
        winSetHomeEnvironment.setVisible(false);
    }

    registerAspect(&gitkOptions);
    gitkOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    gitkOptions.setSettingsKey("GitKOptions");
    gitkOptions.setLabelText(Tr::tr("Arguments:"));

    registerAspect(&logDiff);
    logDiff.setSettingsKey("LogDiff");
    logDiff.setToolTip(Tr::tr("Note that huge amount of commits might take some time."));

    registerAspect(&repositoryBrowserCmd);
    repositoryBrowserCmd.setDisplayStyle(StringAspect::PathChooserDisplay);
    repositoryBrowserCmd.setSettingsKey("RepositoryBrowserCmd");
    repositoryBrowserCmd.setExpectedKind(PathChooser::ExistingCommand);
    repositoryBrowserCmd.setHistoryCompleter("Git.RepoCommand.History");
    repositoryBrowserCmd.setDisplayName(Tr::tr("Git Repository Browser Command"));
    repositoryBrowserCmd.setLabelText(Tr::tr("Command:"));

    registerAspect(&instantBlame);
    instantBlame.setSettingsKey("Git Instant");
    instantBlame.setDefaultValue(true);
    instantBlame.setLabelText(Tr::tr("Add instant blame annotations to editor"));
    instantBlame.setToolTip(Tr::tr("Directly annotate each line in the editor "
                                   "when scrolling through the document."));

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
            *errorMessage = Tr::tr("The binary \"%1\" could not be located in the path \"%2\"")
                .arg(binaryPath.value(), path.value());
    }
    return binPath;
}

// GitSettingsPage

GitSettingsPage::GitSettingsPage(GitSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_GIT);
    setDisplayName(Tr::tr("Git"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        GitSettings &s = *settings;
        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Column {
                    Row { s.path },
                    s.winSetHomeEnvironment,
                }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Row { s.logCount, s.timeout, st },
                    s.pullRebase
                }
            },

            Group {
                title(Tr::tr("Gitk")),
                Row { s.gitkOptions }
            },

            Group {
                title(Tr::tr("Repository Browser")),
                Row { s.repositoryBrowserCmd }
            },

            Group {
                title(Tr::tr("Instant Blame")),
                Row { s.instantBlame }
            },

            st
        }.attachTo(widget);
    });
}

} // Git::Internal
