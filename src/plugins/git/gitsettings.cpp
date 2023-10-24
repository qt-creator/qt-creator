// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitsettings.h"

#include "gittr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QDir>

using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

GitSettings &settings()
{
    static GitSettings theSettings;
    return theSettings;
}

GitSettings::GitSettings()
{
    setAutoApply(false);
    setSettingsGroup("Git");

    path.setDisplayStyle(StringAspect::LineEditDisplay);
    path.setLabelText(Tr::tr("Prepend to PATH:"));

    binaryPath.setDefaultValue("git");

    pullRebase.setSettingsKey("PullRebase");
    pullRebase.setLabelText(Tr::tr("Pull with rebase"));

    showTags.setSettingsKey("ShowTags");

    omitAnnotationDate.setSettingsKey("OmitAnnotationDate");

    ignoreSpaceChangesInDiff.setSettingsKey("SpaceIgnorantDiff");
    ignoreSpaceChangesInDiff.setDefaultValue(true);

    ignoreSpaceChangesInBlame.setSettingsKey("SpaceIgnorantBlame");
    ignoreSpaceChangesInBlame.setDefaultValue(true);

    blameMoveDetection.setSettingsKey("BlameDetectMove");
    blameMoveDetection.setDefaultValue(0);

    diffPatience.setSettingsKey("DiffPatience");
    diffPatience.setDefaultValue(true);

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

    gitkOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    gitkOptions.setSettingsKey("GitKOptions");
    gitkOptions.setLabelText(Tr::tr("Arguments:"));

    logDiff.setSettingsKey("LogDiff");
    logDiff.setToolTip(Tr::tr("Note that huge amount of commits might take some time."));

    repositoryBrowserCmd.setSettingsKey("RepositoryBrowserCmd");
    repositoryBrowserCmd.setExpectedKind(PathChooser::ExistingCommand);
    repositoryBrowserCmd.setHistoryCompleter("Git.RepoCommand.History");
    repositoryBrowserCmd.setDisplayName(Tr::tr("Git Repository Browser Command"));
    repositoryBrowserCmd.setLabelText(Tr::tr("Command:"));

    instantBlame.setSettingsKey("Git Instant");
    instantBlame.setDefaultValue(true);
    instantBlame.setLabelText(Tr::tr("Add instant blame annotations to editor"));
    instantBlame.setToolTip(
        Tr::tr("Annotate the current line in the editor with Git \"blame\" output."));
    instantBlameIgnoreSpaceChanges.setSettingsKey("GitInstantIgnoreSpaceChanges");
    instantBlameIgnoreSpaceChanges.setDefaultValue(false);
    instantBlameIgnoreSpaceChanges.setLabelText(trIgnoreWhitespaceChanges());
    instantBlameIgnoreSpaceChanges.setToolTip(
        Tr::tr("Finds the commit that introduced the last real code changes to the line."));
    instantBlameIgnoreLineMoves.setSettingsKey("GitInstantIgnoreLineMoves");
    instantBlameIgnoreLineMoves.setDefaultValue(false);
    instantBlameIgnoreLineMoves.setLabelText(trIgnoreLineMoves());
    instantBlameIgnoreLineMoves.setToolTip(
        Tr::tr("Finds the commit that introduced the line before it was moved."));

    graphLog.setSettingsKey("GraphLog");

    colorLog.setSettingsKey("ColorLog");
    colorLog.setDefaultValue(true);

    firstParent.setSettingsKey("FirstParent");

    followRenames.setSettingsKey("FollowRenames");
    followRenames.setDefaultValue(true);

    lastResetIndex.setSettingsKey("LastResetIndex");

    refLogShowDate.setSettingsKey("RefLogShowDate");

    timeout.setDefaultValue(Utils::HostOsInfo::isWindowsHost() ? 60 : 30);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Configuration")),
                Column {
                    Row { path },
                    winSetHomeEnvironment,
                }
            },

            Group {
                title(Tr::tr("Miscellaneous")),
                Column {
                    Row { logCount, timeout, st },
                    pullRebase
                }
            },

            Group {
                title(Tr::tr("Gitk")),
                Row { gitkOptions }
            },

            Group {
                title(Tr::tr("Repository Browser")),
                Row { repositoryBrowserCmd }
            },

            Group {
                title(Tr::tr("Instant Blame")),
                instantBlame.groupChecker(),
                Row { instantBlameIgnoreSpaceChanges, instantBlameIgnoreLineMoves, st },
            },

            st
        };
    });
    connect(&binaryPath, &BaseAspect::changed, this, [this] { tryResolve = true; });
    connect(&path, &BaseAspect::changed, this, [this] { tryResolve = true; });

    readSettings();
}

FilePath GitSettings::gitExecutable(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default to pathless binary.
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();

    if (tryResolve) {
        resolvedBinPath = binaryPath();
        if (!resolvedBinPath.isAbsolutePath())
            resolvedBinPath = resolvedBinPath.searchInPath(searchPathList(), FilePath::PrependToPath);
        tryResolve = false;
    }

    if (resolvedBinPath.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = Tr::tr("The binary \"%1\" could not be located in the path \"%2\"")
                .arg(binaryPath().toUserOutput(), path());
    }
    return resolvedBinPath;
}

QString GitSettings::trIgnoreWhitespaceChanges()
{
    return Tr::tr("Ignore whitespace changes");
}

QString GitSettings::trIgnoreLineMoves()
{
    return Tr::tr("Ignore line moves");
}

// GitSettingsPage

class GitSettingsPage final : public Core::IOptionsPage
{
public:
    GitSettingsPage()
    {
        setId(VcsBase::Constants::VCS_ID_GIT);
        setDisplayName(Tr::tr("Git"));
        setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const GitSettingsPage settingsPage;

} // Git::Internal
