/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>

using namespace Utils;

namespace QSsh {
namespace Internal {

struct SshSettings
{
    bool useConnectionSharing = !HostOsInfo::isWindowsHost();
    int connectionSharingTimeOutInMinutes = 10;
    FileName sshFilePath;
    FileName sftpFilePath;
    FileName askpassFilePath;
    FileName keygenFilePath;
    QSsh::SshSettings::SearchPathRetriever searchPathRetriever = [] { return FileNameList(); };
};

} // namespace Internal

Q_GLOBAL_STATIC(QSsh::Internal::SshSettings, sshSettings)


class AccessSettingsGroup
{
public:
    AccessSettingsGroup(QSettings *settings) : m_settings(settings)
    {
        settings->beginGroup("SshSettings");
    }
    ~AccessSettingsGroup() { m_settings->endGroup(); }
private:
    QSettings * const m_settings;
};

static QString connectionSharingKey() { return QString("UseConnectionSharing"); }
static QString connectionSharingTimeoutKey() { return QString("ConnectionSharingTimeout"); }
static QString sshFilePathKey() { return QString("SshFilePath"); }
static QString sftpFilePathKey() { return QString("SftpFilePath"); }
static QString askPassFilePathKey() { return QString("AskpassFilePath"); }
static QString keygenFilePathKey() { return QString("KeygenFilePath"); }

void SshSettings::loadSettings(QSettings *settings)
{
    AccessSettingsGroup g(settings);
    QVariant value = settings->value(connectionSharingKey());
    if (value.isValid() && !HostOsInfo::isWindowsHost())
        sshSettings->useConnectionSharing = value.toBool();
    value = settings->value(connectionSharingTimeoutKey());
    if (value.isValid())
        sshSettings->connectionSharingTimeOutInMinutes = value.toInt();
    sshSettings->sshFilePath = FileName::fromString(settings->value(sshFilePathKey()).toString());
    sshSettings->sftpFilePath = FileName::fromString(settings->value(sftpFilePathKey()).toString());
    sshSettings->askpassFilePath = FileName::fromString(
                settings->value(askPassFilePathKey()).toString());
    sshSettings->keygenFilePath = FileName::fromString(
                settings->value(keygenFilePathKey()).toString());
}

void SshSettings::storeSettings(QSettings *settings)
{
    AccessSettingsGroup g(settings);
    settings->setValue(connectionSharingKey(), sshSettings->useConnectionSharing);
    settings->setValue(connectionSharingTimeoutKey(),
                       sshSettings->connectionSharingTimeOutInMinutes);
    settings->setValue(sshFilePathKey(), sshSettings->sshFilePath.toString());
    settings->setValue(sftpFilePathKey(), sshSettings->sftpFilePath.toString());
    settings->setValue(askPassFilePathKey(), sshSettings->askpassFilePath.toString());
    settings->setValue(keygenFilePathKey(), sshSettings->keygenFilePath.toString());
}

void SshSettings::setConnectionSharingEnabled(bool share)
{
    sshSettings->useConnectionSharing = share;
}
bool SshSettings::connectionSharingEnabled() { return sshSettings->useConnectionSharing; }

void SshSettings::setConnectionSharingTimeout(int timeInMinutes)
{
    sshSettings->connectionSharingTimeOutInMinutes = timeInMinutes;
}
int SshSettings::connectionSharingTimeout()
{
    return sshSettings->connectionSharingTimeOutInMinutes;
}

static FileName filePathValue(const FileName &value, const QStringList &candidateFileNames)
{
    if (!value.isEmpty())
        return value;
    const QList<FileName> additionalSearchPaths = sshSettings->searchPathRetriever();
    for (const QString &candidate : candidateFileNames) {
        const FileName filePath = Environment::systemEnvironment()
                .searchInPath(candidate, additionalSearchPaths);
        if (!filePath.isEmpty())
            return filePath;
    }
    return FileName();
}

static FileName filePathValue(const FileName &value, const QString &candidateFileName)
{
    return filePathValue(value, QStringList(candidateFileName));
}

void SshSettings::setSshFilePath(const FileName &ssh) { sshSettings->sshFilePath = ssh; }
FileName SshSettings::sshFilePath() { return filePathValue(sshSettings->sshFilePath, "ssh"); }

void SshSettings::setSftpFilePath(const FileName &sftp) { sshSettings->sftpFilePath = sftp; }
FileName SshSettings::sftpFilePath() { return filePathValue(sshSettings->sftpFilePath, "sftp"); }

void SshSettings::setAskpassFilePath(const FileName &askPass)
{
    sshSettings->askpassFilePath = askPass;
}

FileName SshSettings::askpassFilePath()
{
    FileName candidate;
    candidate = sshSettings->askpassFilePath;
    if (candidate.isEmpty())
        candidate = FileName::fromString(Environment::systemEnvironment().value("SSH_ASKPASS"));
    return filePathValue(candidate, QStringList{"qtc-askpass", "ssh-askpass"});
}

void SshSettings::setKeygenFilePath(const FileName &keygen)
{
    sshSettings->keygenFilePath = keygen;
}

FileName SshSettings::keygenFilePath()
{
    return filePathValue(sshSettings->keygenFilePath, "ssh-keygen");
}

void SshSettings::setExtraSearchPathRetriever(const SearchPathRetriever &pathRetriever)
{
    sshSettings->searchPathRetriever = pathRetriever;
}

} // namespace QSsh
