// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perforcesettings.h"

#include "perforcechecker.h"
#include "perforcetr.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
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

namespace Perforce::Internal {

static QString defaultCommand()
{
    return QLatin1String("p4" QTC_HOST_EXE_SUFFIX);
}

PerforceSettings &settings()
{
    static PerforceSettings theSettings;
    return theSettings;
}

PerforceSettings::PerforceSettings()
{
    setSettingsGroup("Perforce");
    setAutoApply(false);

    p4BinaryPath.setSettingsKey("Command");
    p4BinaryPath.setDefaultValue(
        Environment::systemEnvironment().searchInPath(defaultCommand()).toUserOutput());
    p4BinaryPath.setHistoryCompleter("Perforce.Command.History");
    p4BinaryPath.setExpectedKind(PathChooser::Command);
    p4BinaryPath.setDisplayName(Tr::tr("Perforce Command"));
    p4BinaryPath.setLabelText(Tr::tr("P4 command:"));

    p4Port.setDisplayStyle(StringAspect::LineEditDisplay);
    p4Port.setSettingsKey("Port");
    p4Port.setLabelText(Tr::tr("P4 port:"));

    p4Client.setDisplayStyle(StringAspect::LineEditDisplay);
    p4Client.setSettingsKey("Client");
    p4Client.setLabelText(Tr::tr("P4 client:"));

    p4User.setDisplayStyle(StringAspect::LineEditDisplay);
    p4User.setSettingsKey("User");
    p4User.setLabelText(Tr::tr("P4 user:"));

    logCount.setSettingsKey("LogCount");
    logCount.setRange(1000, 10000);
    logCount.setDefaultValue(1000);
    logCount.setLabelText(Tr::tr("Log count:"));

    // The settings value has been stored with the opposite meaning for a while.
    // Avoid changing the stored value, but flip it on read/write:
    customEnv.setSettingsKey("Default");
    const auto invertBoolVariant = [](const QVariant &v) { return QVariant(!v.toBool()); };
    customEnv.setFromSettingsTransformation(invertBoolVariant);
    customEnv.setToSettingsTransformation(invertBoolVariant);

    timeOutS.setSettingsKey("TimeOut");
    timeOutS.setRange(1, 360);
    timeOutS.setDefaultValue(30);
    timeOutS.setLabelText(Tr::tr("Timeout:"));
    timeOutS.setSuffix(Tr::tr("s"));

    autoOpen.setSettingsKey("PromptToOpen");
    autoOpen.setDefaultValue(true);
    autoOpen.setLabelText(Tr::tr("Automatically open files when editing"));

    setLayouter([this] {
        using namespace Layouting;

        auto errorLabel = new InfoLabel({}, InfoLabel::None);
        errorLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        errorLabel->setFilled(true);
        auto testButton = new QPushButton(Tr::tr("Test"));
        QObject::connect(testButton, &QPushButton::clicked, errorLabel,
                [this, errorLabel, testButton] {
            testButton->setEnabled(false);
            auto checker = new PerforceChecker(errorLabel);
            checker->setUseOverideCursor(true);
            QObject::connect(checker, &PerforceChecker::failed, errorLabel,
                    [errorLabel, testButton, checker](const QString &t) {
                errorLabel->setType(InfoLabel::Error);
                errorLabel->setText(t);
                testButton->setEnabled(true);
                checker->deleteLater();
            });
            QObject::connect(checker, &PerforceChecker::succeeded, errorLabel,
                    [errorLabel, testButton, checker](const FilePath &repo) {
                errorLabel->setType(InfoLabel::Ok);
                errorLabel->setText(Tr::tr("Test succeeded (%1).")
                                        .arg(repo.toUserOutput()));
                testButton->setEnabled(true);
                checker->deleteLater();
            });

            errorLabel->setType(InfoLabel::Information);
            errorLabel->setText(Tr::tr("Testing..."));

            const FilePath p4Bin = FilePath::fromUserInput(p4BinaryPath.volatileValue());
            checker->start(p4Bin, {}, commonP4Arguments_volatile(), 10000);
        });

        Group config {
            title(Tr::tr("Configuration")),
            Row { p4BinaryPath }
        };

        Group environment {
            title(Tr::tr("Environment Variables")),
            customEnv.groupChecker(),
            Row { p4Port, p4Client, p4User }
        };

        Group misc {
            title(Tr::tr("Miscellaneous")),
            Column {
                Row { logCount, timeOutS, st },
                autoOpen
            }
        };

        return Column {
            config,
            environment,
            misc,
            Row { errorLabel, st, testButton },
            st
        };
    });

    readSettings();
}

// --------------------PerforceSettings
PerforceSettings::~PerforceSettings()
{
    delete m_topLevelDir;
}

QStringList PerforceSettings::commonP4Arguments() const
{
    QStringList lst;
    if (customEnv()) {
        if (!p4Client().isEmpty())
            lst << "-c" << p4Client();
        if (!p4Port().isEmpty())
            lst << "-p" << p4Port();
        if (!p4User().isEmpty())
            lst << "-u" << p4User();
    }
    return lst;
}

QStringList PerforceSettings::commonP4Arguments_volatile() const
{
    QStringList lst;
    if (customEnv.volatileValue()) {
        const QString p4C = p4Client.volatileValue();
        if (!p4C.isEmpty())
            lst << "-c" << p4C;
        const QString p4P = p4Port.volatileValue();
        if (!p4P.isEmpty())
            lst << "-p" << p4P;
        const QString p4U = p4User.volatileValue();
        if (!p4U.isEmpty())
            lst << "-u" << p4U;
    }
    return lst;
}

bool PerforceSettings::isValid() const
{
    return !m_topLevel.isEmpty() && !p4BinaryPath().isEmpty();
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

class PerforceSettingsPage final : public Core::IOptionsPage
{
public:
    explicit PerforceSettingsPage()
    {
        setId(VcsBase::Constants::VCS_ID_PERFORCE);
        setDisplayName(Tr::tr("Perforce"));
        setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });

    }
};

const PerforceSettingsPage settingsPage;

} // Perforce::Internal
