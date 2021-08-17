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

#include "perforcesettings.h"

#include "perforcechecker.h"
#include "perforceplugin.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QStringList>

using namespace Utils;

namespace Perforce {
namespace Internal {

static QString defaultCommand()
{
    return QLatin1String("p4" QTC_HOST_EXE_SUFFIX);
}

PerforceSettings::PerforceSettings()
{
    setSettingsGroup("Perforce");
    setAutoApply(false);

    registerAspect(&p4BinaryPath);
    p4BinaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    p4BinaryPath.setSettingsKey("Command");
    p4BinaryPath.setDefaultValue(
            Environment::systemEnvironment().searchInPath(defaultCommand()).toString());
    p4BinaryPath.setHistoryCompleter("Perforce.Command.History");
    p4BinaryPath.setExpectedKind(PathChooser::Command);
    p4BinaryPath.setDisplayName(tr("Perforce Command"));
    p4BinaryPath.setLabelText(tr("P4 command:"));

    registerAspect(&p4Port);
    p4Port.setDisplayStyle(StringAspect::LineEditDisplay);
    p4Port.setSettingsKey("Port");
    p4Port.setLabelText(tr("P4 port:"));

    registerAspect(&p4Client);
    p4Client.setDisplayStyle(StringAspect::LineEditDisplay);
    p4Client.setSettingsKey("Client");
    p4Client.setLabelText(tr("P4 client:"));

    registerAspect(&p4User);
    p4User.setDisplayStyle(StringAspect::LineEditDisplay);
    p4User.setSettingsKey("User");
    p4User.setLabelText(tr("P4 user:"));

    registerAspect(&logCount);
    logCount.setSettingsKey("LogCount");
    logCount.setRange(1000, 10000);
    logCount.setDefaultValue(1000);
    logCount.setLabelText(tr("Log count:"));

    registerAspect(&customEnv);
    // The settings value has been stored with the opposite meaning for a while.
    // Avoid changing the stored value, but flip it on read/write:
    customEnv.setSettingsKey("Default");
    const auto invertBoolVariant = [](const QVariant &v) { return QVariant(!v.toBool()); };
    customEnv.setFromSettingsTransformation(invertBoolVariant);
    customEnv.setToSettingsTransformation(invertBoolVariant);

    registerAspect(&timeOutS);
    timeOutS.setSettingsKey("TimeOut");
    timeOutS.setRange(1, 360);
    timeOutS.setDefaultValue(30);
    timeOutS.setLabelText(tr("Timeout:"));
    timeOutS.setSuffix(tr("s"));

    registerAspect(&promptToSubmit);
    promptToSubmit.setSettingsKey("PromptForSubmit");
    promptToSubmit.setDefaultValue(true);
    promptToSubmit.setLabelText(tr("Prompt on submit"));

    registerAspect(&autoOpen);
    autoOpen.setSettingsKey("PromptToOpen");
    autoOpen.setDefaultValue(true);
    autoOpen.setLabelText(tr("Automatically open files when editing"));
}

// --------------------PerforceSettings
PerforceSettings::~PerforceSettings()
{
    delete m_topLevelDir;
}

QStringList PerforceSettings::commonP4Arguments() const
{
    QStringList lst;
    if (customEnv.value()) {
        if (!p4Client.value().isEmpty())
            lst << "-c" << p4Client.value();
        if (!p4Port.value().isEmpty())
            lst << "-p" << p4Port.value();
        if (!p4User.value().isEmpty())
            lst << "-u" << p4User.value();
    }
    return lst;
}

bool PerforceSettings::isValid() const
{
    return !m_topLevel.isEmpty() && !p4BinaryPath.value().isEmpty();
}

bool PerforceSettings::defaultEnv() const
{
    return !customEnv.value(); // Note: negated
}

FilePath PerforceSettings::topLevel() const
{
    return FilePath::fromString(m_topLevel);
}

FilePath PerforceSettings::topLevelSymLinkTarget() const
{
    return FilePath::fromString(m_topLevelSymLinkTarget);
}

void PerforceSettings::setTopLevel(const QString &t)
{
    if (m_topLevel == t)
        return;
    clearTopLevel();
    if (!t.isEmpty()) {
        // Check/expand symlinks as creator always has expanded file paths
        QFileInfo fi(t);
        if (fi.isSymLink()) {
            m_topLevel = t;
            m_topLevelSymLinkTarget = QFileInfo(fi.symLinkTarget()).absoluteFilePath();
        } else {
            m_topLevelSymLinkTarget = m_topLevel = t;
        }
        m_topLevelDir =  new QDir(m_topLevelSymLinkTarget);
    }
}

void PerforceSettings::clearTopLevel()
{
    delete m_topLevelDir;
    m_topLevelDir = nullptr;
    m_topLevel.clear();
}

QString PerforceSettings::relativeToTopLevel(const QString &dir) const
{
    QTC_ASSERT(m_topLevelDir, return QLatin1String("../") + dir);
    return m_topLevelDir->relativeFilePath(dir);
}

QString PerforceSettings::relativeToTopLevelArguments(const QString &dir) const
{
    return relativeToTopLevel(dir);
}

// Map the root part of a path:
// Calling "/home/john/foo" with old="/home", new="/user"
// results in "/user/john/foo"

static inline QString mapPathRoot(const QString &path,
                                  const QString &oldPrefix,
                                  const QString &newPrefix)
{
    if (path.isEmpty() || oldPrefix.isEmpty() || newPrefix.isEmpty() || oldPrefix == newPrefix)
        return path;
    if (path == oldPrefix)
        return newPrefix;
    if (path.startsWith(oldPrefix))
        return newPrefix + path.right(path.size() - oldPrefix.size());
    return path;
}

QStringList PerforceSettings::commonP4Arguments(const QString &workingDir) const
{
    QStringList rc;
    if (!workingDir.isEmpty()) {
        /* Determine the -d argument for the working directory for matching relative paths.
         * It is is below the toplevel, replace top level portion by exact specification. */
        rc << QLatin1String("-d")
           << QDir::toNativeSeparators(mapPathRoot(workingDir, m_topLevelSymLinkTarget, m_topLevel));
    }
    rc.append(commonP4Arguments());
    return rc;
}

QString PerforceSettings::mapToFileSystem(const QString &perforceFilePath) const
{
    return mapPathRoot(perforceFilePath, m_topLevel, m_topLevelSymLinkTarget);
}

// SettingsPage

PerforceSettingsPage::PerforceSettingsPage(PerforceSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_PERFORCE);
    setDisplayName(PerforceSettings::tr("Perforce"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings, this](QWidget *widget) {
        PerforceSettings &s = *settings;
        using namespace Layouting;

        auto errorLabel = new QLabel;
        auto testButton = new QPushButton(PerforceSettings::tr("Test"));
        connect(testButton, &QPushButton::clicked, this, [this, settings, errorLabel, testButton] {
            testButton->setEnabled(false);
            auto checker = new PerforceChecker(errorLabel);
            checker->setUseOverideCursor(true);
            connect(checker, &PerforceChecker::failed, errorLabel,
                    [errorLabel, testButton, checker](const QString &t) {
                errorLabel->setStyleSheet("background-color: red");
                errorLabel->setText(t);
                testButton->setEnabled(true);
                checker->deleteLater();
            });
            connect(checker, &PerforceChecker::succeeded, errorLabel,
                    [errorLabel, testButton, checker](const FilePath &repo) {
                errorLabel->setStyleSheet({});
                errorLabel->setText(PerforceSettings::tr("Test succeeded (%1).")
                                        .arg(repo.toUserOutput()));
                testButton->setEnabled(true);
                checker->deleteLater();
            });

            errorLabel->setStyleSheet(QString());
            errorLabel->setText(PerforceSettings::tr("Testing..."));
            checker->start(settings->p4BinaryPath.filePath(), {}, settings->commonP4Arguments(), 10000);
        });

        Group config {
            Title(PerforceSettings::tr("Configuration")),
            Row { s.p4BinaryPath }
        };

        Group environment {
            Title(PerforceSettings::tr("Environment Variables"), &s.customEnv),
            Row { s.p4Port, s.p4Client, s.p4User }
        };

        Group misc {
            Title(PerforceSettings::tr("Miscellaneous")),
            Row { s.logCount, s.timeOutS, Stretch() },
            s.promptToSubmit,
            s.autoOpen
        };

        Column {
            config,
            environment,
            misc,
            Row { errorLabel, Stretch(), testButton },
            Stretch()
        }.attachTo(widget);
    });
}

} // Internal
} // Perforce
