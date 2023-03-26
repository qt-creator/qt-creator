// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QPointer>
#include <QVersionNumber>

namespace QbsProjectManager {
namespace Internal {

class QbsSettingsData {
public:
    Utils::FilePath qbsExecutableFilePath;
    QString defaultInstallDirTemplate;
    QVersionNumber qbsVersion; // Ephemeral
    bool useCreatorSettings = true;
};

class QbsSettings : public QObject
{
    Q_OBJECT
public:
    static QbsSettings &instance();

    static Utils::FilePath qbsExecutableFilePath();
    static Utils::FilePath qbsConfigFilePath();
    static bool hasQbsExecutable();
    static QString defaultInstallDirTemplate();
    static bool useCreatorSettingsDirForQbs();
    static QString qbsSettingsBaseDir();
    static QVersionNumber qbsVersion();

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
    Q_OBJECT
public:
    QbsSettingsPage();

private:
    QWidget *widget() override;
    void apply() override;
    void finish() override;

    class SettingsWidget;
    QPointer<SettingsWidget> m_widget;
};

} // namespace Internal
} // namespace QbsProjectManager
