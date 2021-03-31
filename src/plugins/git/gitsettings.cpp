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

#include "gitsettings.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
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
        const QByteArray currentHome = qgetenv("HOME");
        const QString toolTip
                = tr("Set the environment variable HOME to \"%1\"\n(%2).\n"
                     "This causes Git to look for the SSH-keys in that location\n"
                     "instead of its installation directory when run outside git bash.").
                arg(QDir::homePath(),
                    currentHome.isEmpty() ? tr("not currently set") :
                            tr("currently set to \"%1\"").arg(QString::fromLocal8Bit(currentHome)));
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
    if (binPath.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = tr("The binary \"%1\" could not be located in the path \"%2\"")
                .arg(binaryPath.value(), path.value());
    }
    return binPath;
}


// GitSettingsPageWidget

class GitSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::SettingsPageWidget)

public:
    GitSettingsPageWidget(GitSettings *settings, const std::function<void()> &onChange);

    void apply() final;

private:
    std::function<void()> m_onChange;
    GitSettings *m_settings;
};

GitSettingsPageWidget::GitSettingsPageWidget(GitSettings *settings, const std::function<void()> &onChange)
    : m_onChange(onChange), m_settings(settings)
{
    GitSettings &s = *m_settings;
    using namespace Layouting;

    Column {
        Group {
            Title(tr("Configuration")),
            Row { s.path },
            s.winSetHomeEnvironment,
        },

        Group {
            Title(tr("Miscellaneous")),
            Row { s.logCount, s.timeout, Stretch() },
            s.pullRebase
        },

        Group {
            Title(tr("Gitk")),
            Row { s.gitkOptions }
        },

        Group {
            Title(tr("Repository Browser")),
            Row { s.repositoryBrowserCmd }
        },

        Stretch()
    }.attachTo(this);
}

void GitSettingsPageWidget::apply()
{
    if (m_settings->isDirty()) {
        m_settings->apply();
        m_onChange();
    }
}


// GitSettingsPage

GitSettingsPage::GitSettingsPage(GitSettings *settings, const std::function<void()> &onChange)
{
    setId(VcsBase::Constants::VCS_ID_GIT);
    setDisplayName(GitSettingsPageWidget::tr("Git"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([settings, onChange] { return new GitSettingsPageWidget(settings, onChange); });
}

} // namespace Internal
} // namespace Git
