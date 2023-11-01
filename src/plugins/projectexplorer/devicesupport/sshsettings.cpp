// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcsettings.h>

#include <QReadWriteLock>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

struct SshSettings
{
    bool useConnectionSharing = !HostOsInfo::isWindowsHost();
    int connectionSharingTimeOutInMinutes = 10;
    FilePath sshFilePath;
    FilePath sftpFilePath;
    FilePath askpassFilePath;
    FilePath keygenFilePath;
    ProjectExplorer::SshSettings::SearchPathRetriever searchPathRetriever = [] { return FilePaths(); };
    QReadWriteLock lock;
};

} // namespace Internal

Q_GLOBAL_STATIC(Internal::SshSettings, sshSettings)

class AccessSettingsGroup
{
public:
    AccessSettingsGroup(QtcSettings *settings) : m_settings(settings)
    {
        settings->beginGroup("SshSettings");
    }
    ~AccessSettingsGroup() { m_settings->endGroup(); }
private:
    QtcSettings * const m_settings;
};

static Key connectionSharingKey() { return Key("UseConnectionSharing"); }
static Key connectionSharingTimeoutKey() { return Key("ConnectionSharingTimeout"); }
static Key sshFilePathKey() { return Key("SshFilePath"); }
static Key sftpFilePathKey() { return Key("SftpFilePath"); }
static Key askPassFilePathKey() { return Key("AskpassFilePath"); }
static Key keygenFilePathKey() { return Key("KeygenFilePath"); }

void SshSettings::loadSettings(QtcSettings *settings)
{
    QWriteLocker locker(&sshSettings->lock);
    AccessSettingsGroup g(settings);
    QVariant value = settings->value(connectionSharingKey());
    if (value.isValid() && !HostOsInfo::isWindowsHost())
        sshSettings->useConnectionSharing = value.toBool();
    value = settings->value(connectionSharingTimeoutKey());
    if (value.isValid())
        sshSettings->connectionSharingTimeOutInMinutes = value.toInt();
    sshSettings->sshFilePath = FilePath::fromString(settings->value(sshFilePathKey()).toString());
    sshSettings->sftpFilePath = FilePath::fromString(settings->value(sftpFilePathKey()).toString());
    sshSettings->askpassFilePath = FilePath::fromString(
                settings->value(askPassFilePathKey()).toString());
    sshSettings->keygenFilePath = FilePath::fromString(
                settings->value(keygenFilePathKey()).toString());
}

void SshSettings::storeSettings(QtcSettings *settings)
{
    QReadLocker locker(&sshSettings->lock);
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
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->useConnectionSharing = share;
}
bool SshSettings::connectionSharingEnabled()
{
    QReadLocker locker(&sshSettings->lock);
    return sshSettings->useConnectionSharing;
}

void SshSettings::setConnectionSharingTimeout(int timeInMinutes)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->connectionSharingTimeOutInMinutes = timeInMinutes;
}
int SshSettings::connectionSharingTimeout()
{
    QReadLocker locker(&sshSettings->lock);
    return sshSettings->connectionSharingTimeOutInMinutes;
}

// Keep read locker locked while calling this method
static FilePath filePathValue(const FilePath &value, const QStringList &candidateFileNames)
{
    if (!value.isEmpty())
        return value;
    Environment env = Environment::systemEnvironment();
    env.prependToPath(sshSettings->searchPathRetriever());
    for (const QString &candidate : candidateFileNames) {
        const FilePath filePath = env.searchInPath(candidate);
        if (!filePath.isEmpty())
            return filePath;
    }
    return {};
}

// Keep read locker locked while calling this method
static FilePath filePathValue(const FilePath &value, const QString &candidateFileName)
{
    return filePathValue(value, QStringList(candidateFileName));
}

void SshSettings::setSshFilePath(const FilePath &ssh)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->sshFilePath = ssh;
}

FilePath SshSettings::sshFilePath()
{
    QReadLocker locker(&sshSettings->lock);
    return filePathValue(sshSettings->sshFilePath, "ssh");
}

void SshSettings::setSftpFilePath(const FilePath &sftp)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->sftpFilePath = sftp;
}

FilePath SshSettings::sftpFilePath()
{
    QReadLocker locker(&sshSettings->lock);
    return filePathValue(sshSettings->sftpFilePath, "sftp");
}

void SshSettings::setAskpassFilePath(const FilePath &askPass)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->askpassFilePath = askPass;
}

FilePath SshSettings::askpassFilePath()
{
    QReadLocker locker(&sshSettings->lock);
    FilePath candidate;
    candidate = sshSettings->askpassFilePath;
    if (candidate.isEmpty())
        candidate = FilePath::fromString(Environment::systemEnvironment().value("SSH_ASKPASS"));
    return filePathValue(candidate, QStringList{"qtc-askpass", "ssh-askpass"});
}

void SshSettings::setKeygenFilePath(const FilePath &keygen)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->keygenFilePath = keygen;
}

FilePath SshSettings::keygenFilePath()
{
    QReadLocker locker(&sshSettings->lock);
    return filePathValue(sshSettings->keygenFilePath, "ssh-keygen");
}

void SshSettings::setExtraSearchPathRetriever(const SearchPathRetriever &pathRetriever)
{
    QWriteLocker locker(&sshSettings->lock);
    sshSettings->searchPathRetriever = pathRetriever;
}

} // namespace ProjectExplorer
