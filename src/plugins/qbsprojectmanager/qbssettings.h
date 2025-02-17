// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/filepath.h>

#include <QVersionNumber>

namespace ProjectExplorer { class Kit; }
namespace Utils { class Environment; }

namespace QbsProjectManager::Internal {

class QbsSettingsData
{
public:
    Utils::FilePath qbsExecutableFilePath;
    QString defaultInstallDirTemplate;
    bool useCreatorSettings = true;
};

class QbsSettings : public QObject
{
    Q_OBJECT
public:
    static QbsSettings &instance();

    static Utils::FilePath qbsExecutableFilePath(const ProjectExplorer::Kit &kit);
    static Utils::FilePath qbsExecutableFilePath(const ProjectExplorer::IDeviceConstPtr &device);
    static Utils::FilePath defaultQbsExecutableFilePath();
    static Utils::FilePath qbsConfigFilePath(const ProjectExplorer::IDeviceConstPtr &device);
    static Utils::Environment qbsProcessEnvironment(const ProjectExplorer::IDeviceConstPtr &device);
    static bool hasQbsExecutable();
    static QString defaultInstallDirTemplate();
    static bool useCreatorSettingsDirForQbs(const ProjectExplorer::IDeviceConstPtr &device);
    static Utils::FilePath qbsSettingsBaseDir(const ProjectExplorer::IDeviceConstPtr &device);
    static QVersionNumber qbsVersion(const ProjectExplorer::IDeviceConstPtr &device);

    static void setSettingsData(const QbsSettingsData &settings);
    static QbsSettingsData rawSettingsData();

signals:
    void settingsChanged();

private:
    QbsSettings();
    void loadSettings();
    void storeSettings() const;

    QbsSettingsData m_settings;
};

class QbsSettingsPage : public Core::IOptionsPage
{
public:
    QbsSettingsPage();
};

} // QbsProjectManager::Internal
