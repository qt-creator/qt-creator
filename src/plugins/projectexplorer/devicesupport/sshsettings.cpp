// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshsettings.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QWriteLocker>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

SshSettings &sshSettings()
{
    static SshSettings theSshSettings;
    return theSshSettings;
}

static FilePaths extraSearchPaths()
{
    FilePaths searchPaths = {ICore::libexecPath()};
    if (HostOsInfo::isWindowsHost()) {
        const QString gitBinary = ICore::settings()->value("Git/BinaryPath", "git")
                                      .toString();
        const QStringList rawGitSearchPaths = ICore::settings()->value("Git/Path")
                                                  .toString().split(':', Qt::SkipEmptyParts);
        const FilePaths gitSearchPaths = Utils::transform(rawGitSearchPaths,
                  [](const QString &rawPath) { return FilePath::fromUserInput(rawPath); });
        const FilePath fullGitPath = Environment::systemEnvironment()
                                         .searchInPath(gitBinary, gitSearchPaths);
        if (!fullGitPath.isEmpty()) {
            searchPaths << fullGitPath.parentDir()
                        << fullGitPath.parentDir().parentDir().pathAppended("usr/bin");
        }
    }
    return searchPaths;
}

static FilePath filePathValue(const FilePath &value, const QStringList &candidateFileNames)
{
    if (!value.isEmpty())
        return value;
    Environment env = Environment::systemEnvironment();
    env.prependToPath(extraSearchPaths());
    for (const QString &candidate : candidateFileNames) {
        const FilePath filePath = env.searchInPath(candidate);
        if (!filePath.isEmpty())
            return filePath;
    }
    return {};
}

SshSettings::SshSettings()
{
    setSettingsGroup("SshSettings");
    setAutoApply(false);

    setLayouter([this] {
        using namespace Layouting;
        return Form {
            m_useConnectionSharingAspect, br,
            m_connectionSharingTimeoutInMinutesAspect, br,
            m_sshFilePathAspect, br,
            m_sftpFilePathAspect, br,
            m_askpassFilePathAspect, br,
            m_keygenFilePathAspect, br
        };
    });

    m_useConnectionSharingAspect.setSettingsKey("UseConnectionSharing");
    m_useConnectionSharingAspect.setDefaultValue(!HostOsInfo::isWindowsHost());
    m_useConnectionSharingAspect.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    m_useConnectionSharingAspect.setLabelText(Tr::tr("Enable connection sharing:"));

    m_connectionSharingTimeoutInMinutesAspect.setSettingsKey("ConnectionSharingTimeout");
    m_connectionSharingTimeoutInMinutesAspect.setDefaultValue(10);
    m_connectionSharingTimeoutInMinutesAspect.setLabelText(Tr::tr("Connection sharing timeout:"));
    m_connectionSharingTimeoutInMinutesAspect.setRange(1, 1'000'000);
    m_connectionSharingTimeoutInMinutesAspect.setSuffix(Tr::tr(" minutes"));

    m_sshFilePathAspect.setSettingsKey("SshFilePath");
    m_sshFilePathAspect.setLabelText(Tr::tr("Path to ssh executable:"));

    m_sftpFilePathAspect.setSettingsKey("SftpFilePath");
    m_sftpFilePathAspect.setLabelText(Tr::tr("Path to sftp executable:"));

    m_askpassFilePathAspect.setSettingsKey("AskpassFilePath");
    m_askpassFilePathAspect.setLabelText(Tr::tr("Path to ssh-askpass executable:"));

    m_keygenFilePathAspect.setSettingsKey("KeygenFilePath");
    m_keygenFilePathAspect.setLabelText(Tr::tr("Path to ssh-keygen executable:"));

    readSettings();

    if (m_sshFilePathAspect().isEmpty())
        m_sshFilePathAspect.setDefaultPathValue(filePathValue("", {"ssh"}));

    if (m_sftpFilePathAspect().isEmpty())
        m_sftpFilePathAspect.setDefaultPathValue(filePathValue("", {"sftp"}));

    if (m_keygenFilePathAspect().isEmpty())
        m_keygenFilePathAspect.setDefaultPathValue(filePathValue("", {"ssh-keygen"}));

    if (m_askpassFilePathAspect().isEmpty()) {
        const FilePath systemSshAskPass =
            FilePath::fromString(Environment::systemEnvironment().value("SSH_ASKPASS"));
        m_askpassFilePathAspect.setDefaultPathValue(
            filePathValue(systemSshAskPass, QStringList{"qtc-askpass", "ssh-askpass"}));
    }

    m_connectionSharingTimeoutInMinutesAspect.setEnabler(&m_useConnectionSharingAspect);
}

FilePath SshSettings::sshFilePath() const
{
    QWriteLocker lock(&m_lock);
    return m_sshFilePathAspect();
}

FilePath SshSettings::askpassFilePath() const
{
    QWriteLocker lock(&m_lock);
    return m_askpassFilePathAspect();
}

FilePath SshSettings::keygenFilePath() const
{
    QWriteLocker lock(&m_lock);
    return m_keygenFilePathAspect();
}

FilePath SshSettings::sftpFilePath() const
{
    QWriteLocker lock(&m_lock);
    return m_sftpFilePathAspect();
}

bool SshSettings::useConnectionSharing() const
{
    QWriteLocker lock(&m_lock);
    return m_useConnectionSharingAspect();
}

int SshSettings::connectionSharingTimeoutInMinutes() const
{
    QWriteLocker lock(&m_lock);
    return m_connectionSharingTimeoutInMinutesAspect();
}

// SshSettingsWidget

class SshSettingsWidget : public IOptionsPageWidget
{
public:
    SshSettingsWidget()
    {
        QWriteLocker locker(&sshSettings().m_lock);
        sshSettings().layouter()().attachTo(this);

        installMarkSettingsDirtyTriggerRecursively(this);
    }

    void apply()
    {
        QWriteLocker lock(&sshSettings().m_lock);
        sshSettings().apply();
        sshSettings().writeSettings();
    }

    void cancel()
    {
        QWriteLocker lock(&sshSettings().m_lock);
        sshSettings().cancel();
    }
};

// SshSettingsPage

class SshSettingsPage final : public Core::IOptionsPage
{
public:
    SshSettingsPage()
    {
        setId(Constants::SSH_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("SSH"));
        setCategory(Constants::DEVICE_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new SshSettingsWidget; });
    }
};

static SshSettingsPage theSshSettingsPage;

} // namespace ProjectExplorer
