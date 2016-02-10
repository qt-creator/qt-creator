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
#include "perforceplugin.h"

#include <utils/qtcassert.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

static const char groupC[] = "Perforce";
static const char commandKeyC[] = "Command";
static const char defaultKeyC[] = "Default";
static const char portKeyC[] = "Port";
static const char clientKeyC[] = "Client";
static const char userKeyC[] = "User";
static const char promptToSubmitKeyC[] = "PromptForSubmit";
static const char autoOpenKeyC[] = "PromptToOpen";
static const char timeOutKeyC[] = "TimeOut";
static const char logCountKeyC[] = "LogCount";

enum { defaultTimeOutS = 30, defaultLogCount = 1000 };

static QString defaultCommand()
{
    return QLatin1String("p4" QTC_HOST_EXE_SUFFIX);
}

namespace Perforce {
namespace Internal {

Settings::Settings() : logCount(defaultLogCount), timeOutS(defaultTimeOutS)
{ }

bool Settings::equals(const Settings &rhs) const
{
    return defaultEnv == rhs.defaultEnv
            && logCount == rhs.logCount
            && p4Command == rhs.p4Command && p4Port == rhs.p4Port
            && p4Client == rhs.p4Client && p4User == rhs.p4User
            && timeOutS == rhs.timeOutS && promptToSubmit == rhs.promptToSubmit
            && autoOpen == rhs.autoOpen;
}

QStringList Settings::commonP4Arguments() const
{
    if (defaultEnv)
        return QStringList();
    QStringList lst;
    if (!p4Client.isEmpty())
        lst << QLatin1String("-c") << p4Client;
    if (!p4Port.isEmpty())
        lst << QLatin1String("-p") << p4Port;
    if (!p4User.isEmpty())
        lst << QLatin1String("-u") << p4User;
    return lst;
}

// --------------------PerforceSettings
PerforceSettings::~PerforceSettings()
{
    delete m_topLevelDir;
}

void PerforceSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    m_settings.p4Command = settings->value(QLatin1String(commandKeyC), defaultCommand()).toString();
    m_settings.p4BinaryPath =
            Utils::Environment::systemEnvironment().searchInPath(m_settings.p4Command).toString();
    m_settings.defaultEnv = settings->value(QLatin1String(defaultKeyC), true).toBool();
    m_settings.p4Port = settings->value(QLatin1String(portKeyC), QString()).toString();
    m_settings.p4Client = settings->value(QLatin1String(clientKeyC), QString()).toString();
    m_settings.p4User = settings->value(QLatin1String(userKeyC), QString()).toString();
    m_settings.timeOutS = settings->value(QLatin1String(timeOutKeyC), defaultTimeOutS).toInt();
    m_settings.promptToSubmit = settings->value(QLatin1String(promptToSubmitKeyC), true).toBool();
    m_settings.logCount = settings->value(QLatin1String(logCountKeyC), int(defaultLogCount)).toInt();
    m_settings.autoOpen = settings->value(QLatin1String(autoOpenKeyC), true).toBool();
    settings->endGroup();
}

void PerforceSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(commandKeyC), m_settings.p4Command);
    settings->setValue(QLatin1String(defaultKeyC), m_settings.defaultEnv);
    settings->setValue(QLatin1String(portKeyC), m_settings.p4Port);
    settings->setValue(QLatin1String(clientKeyC), m_settings.p4Client);
    settings->setValue(QLatin1String(userKeyC), m_settings.p4User);
    settings->setValue(QLatin1String(timeOutKeyC), m_settings.timeOutS);
    settings->setValue(QLatin1String(promptToSubmitKeyC), m_settings.promptToSubmit);
    settings->setValue(QLatin1String(logCountKeyC), m_settings.logCount);
    settings->setValue(QLatin1String(autoOpenKeyC), m_settings.autoOpen);
    settings->endGroup();
}

void PerforceSettings::setSettings(const Settings &newSettings)
{
    if (newSettings != m_settings) {
        m_settings = newSettings;
        clearTopLevel();
    }
}

Settings PerforceSettings::settings() const
{
    return m_settings;
}

QString PerforceSettings::p4Command() const
{
    return m_settings.p4Command;
}

QString PerforceSettings::p4BinaryPath() const
{
    return m_settings.p4BinaryPath;
}

QString PerforceSettings::p4Port() const
{
    return m_settings.p4Port;
}

QString PerforceSettings::p4Client() const
{
    return m_settings.p4Client;
}

QString PerforceSettings::p4User() const
{
    return m_settings.p4User;
}

bool PerforceSettings::defaultEnv() const
{
    return m_settings.defaultEnv;
}

bool PerforceSettings::promptToSubmit() const
{
    return m_settings.promptToSubmit;
}

void PerforceSettings::setPromptToSubmit(bool p)
{
    m_settings.promptToSubmit = p;
}

bool PerforceSettings::autoOpen() const
{
    return m_settings.autoOpen;
}

void PerforceSettings::setAutoOpen(bool b)
{
    m_settings.autoOpen = b;
}

QString PerforceSettings::topLevel() const
{
    return m_topLevel;
}

QString PerforceSettings::topLevelSymLinkTarget() const
{
    return m_topLevelSymLinkTarget;
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
    rc.append(m_settings.commonP4Arguments());
    return rc;
}

QString PerforceSettings::mapToFileSystem(const QString &perforceFilePath) const
{
    return mapPathRoot(perforceFilePath, m_topLevel, m_topLevelSymLinkTarget);
}

} // Internal
} // Perforce
