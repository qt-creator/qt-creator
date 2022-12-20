// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>

#include <utils/filepath.h>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVersionNumber>

#include <memory>

QT_BEGIN_NAMESPACE
class QSettings;
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Ios::Internal {

class DevelopmentTeam;

class ProvisioningProfile
{
public:
    std::shared_ptr<DevelopmentTeam> developmentTeam() { return m_team; }
    QString identifier() const;
    QString displayName() const;
    QString details() const;
    const QDateTime &expirationDate() const { return m_expirationDate; }

private:
    std::shared_ptr<DevelopmentTeam> m_team;
    QString m_identifier;
    QString m_name;
    QString m_appID;
    QDateTime m_expirationDate;
    friend class IosConfigurations;
    friend QDebug &operator<<(QDebug &stream, std::shared_ptr<ProvisioningProfile> profile);
};

using ProvisioningProfilePtr = std::shared_ptr<ProvisioningProfile>;
using ProvisioningProfiles = QList<ProvisioningProfilePtr>;

class DevelopmentTeam
{
public:
    QString identifier() const;
    QString displayName() const;
    QString details() const;
    bool isFreeProfile() const { return m_freeTeam; }
    bool hasProvisioningProfile() const { return !m_profiles.isEmpty(); }

private:
    QString m_identifier;
    QString m_name;
    QString m_email;
    bool m_freeTeam = false;
    ProvisioningProfiles m_profiles;
    friend class IosConfigurations;
    friend QDebug &operator<<(QDebug &stream, std::shared_ptr<DevelopmentTeam> team);
};

using DevelopmentTeamPtr = std::shared_ptr<DevelopmentTeam>;
using DevelopmentTeams = QList<DevelopmentTeamPtr>;

class IosToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    IosToolChainFactory();

    ProjectExplorer::Toolchains autoDetect(
            const ProjectExplorer::ToolchainDetector &detector) const final;
};

class IosConfigurations : public QObject
{
    Q_OBJECT

public:
    static IosConfigurations *instance();
    static void initialize();
    static bool ignoreAllDevices();
    static void setIgnoreAllDevices(bool ignoreDevices);
    static void setScreenshotDir(const Utils::FilePath &path);
    static Utils::FilePath screenshotDir();
    static Utils::FilePath developerPath();
    static QVersionNumber xcodeVersion();
    static Utils::FilePath lldbPath();
    static void updateAutomaticKitList();
    static const DevelopmentTeams &developmentTeams();
    static DevelopmentTeamPtr developmentTeam(const QString &teamID);
    static const ProvisioningProfiles &provisioningProfiles();
    static ProvisioningProfilePtr provisioningProfile(const QString &profileID);

signals:
    void provisioningDataChanged();

private:
    IosConfigurations(QObject *parent);
    void load();
    void save();
    void kitsRestored();
    void updateSimulators();
    static void setDeveloperPath(const Utils::FilePath &devPath);
    void initializeProvisioningData();
    void loadProvisioningData(bool notify = true);

    Utils::FilePath m_developerPath;
    Utils::FilePath m_screenshotDir;
    QVersionNumber m_xcodeVersion;
    bool m_ignoreAllDevices;
    QFileSystemWatcher *m_provisioningDataWatcher = nullptr;
    ProvisioningProfiles m_provisioningProfiles;
    DevelopmentTeams m_developerTeams;
};

QDebug &operator<<(QDebug &stream, std::shared_ptr<ProvisioningProfile> profile);
QDebug &operator<<(QDebug &stream, std::shared_ptr<DevelopmentTeam> team);

} // Ios::Internal
