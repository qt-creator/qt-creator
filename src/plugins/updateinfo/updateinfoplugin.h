// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QDate;
QT_END_NAMESPACE

namespace UpdateInfo {

namespace Internal {

const char FILTER_OPTIONS_PAGE_ID[] = "Update";

class UpdateInfoPluginPrivate;

class UpdateInfoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "UpdateInfo.json")
public:
    enum CheckUpdateInterval {
        DailyCheck,
        WeeklyCheck,
        MonthlyCheck
    };
    Q_ENUM(CheckUpdateInterval)

    UpdateInfoPlugin();
    ~UpdateInfoPlugin() override;

    void extensionsInitialized() override;
    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    bool isAutomaticCheck() const;
    void setAutomaticCheck(bool on);

    CheckUpdateInterval checkUpdateInterval() const;
    void setCheckUpdateInterval(CheckUpdateInterval interval);

    bool isCheckingForQtVersions() const;
    void setCheckingForQtVersions(bool on);

    QDate lastCheckDate() const;
    QDate nextCheckDate() const;
    QDate nextCheckDate(CheckUpdateInterval interval) const;

    bool isCheckForUpdatesRunning() const;
    void startCheckForUpdates();

signals:
    void lastCheckDateChanged(const QDate &date);
    void newUpdatesAvailable(bool available);
    void checkForUpdatesRunningChanged(bool running);

private:
    void setLastCheckDate(const QDate &date);

    void startAutoCheckForUpdates();
    void stopAutoCheckForUpdates();
    void doAutoCheckForUpdates();

    void startMaintenanceTool(const QStringList &args) const;
    void startUpdater() const;
    void startPackageManager() const;
    void stopCheckForUpdates();
    void checkForUpdatesStopped();

    void checkForUpdatesFinished();

    void loadSettings() const;
    void saveSettings();

private:
    UpdateInfoPluginPrivate *d;
};

} // namespace Internal
} // namespace UpdateInfo
