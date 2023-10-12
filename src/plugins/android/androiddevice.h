// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androiddeviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QFutureWatcher>
#include <QFileSystemWatcher>
#include <QSettings>

namespace Utils { class Process; }

namespace Android {
namespace Internal {

class AndroidDevice final : public ProjectExplorer::IDevice
{
public:
    AndroidDevice();

    static IDevice::Ptr create();
    static AndroidDeviceInfo androidDeviceInfoFromIDevice(const IDevice *dev);

    static QString displayNameFromInfo(const AndroidDeviceInfo &info);
    static Utils::Id idFromDeviceInfo(const AndroidDeviceInfo &info);
    static Utils::Id idFromAvdInfo(const CreateAvdInfo &info);

    QStringList supportedAbis() const;
    bool canSupportAbis(const QStringList &abis) const;

    bool canHandleDeployments() const;

    bool isValid() const;
    QString serialNumber() const;
    QString avdName() const;
    int sdkLevel() const;

    Utils::FilePath avdPath() const;
    void setAvdPath(const Utils::FilePath &path);

    QString deviceTypeName() const;
    QString androidVersion() const;

    // AVD specific
    QString skinName() const;
    QString androidTargetName() const;
    QString sdcardSize() const;
    QString openGLStatus() const;

protected:
    void fromMap(const Utils::Store &map) final;

private:
    void addActionsIfNotFound();
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;

    QSettings *avdSettings() const;
    void initAvdSettings();

    std::unique_ptr<QSettings> m_avdSettings;
};

class AndroidDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    AndroidDeviceFactory();

private:
    const AndroidConfig &m_androidConfig;
};

class AndroidDeviceManager : public QObject
{
public:
    static AndroidDeviceManager *instance();
    void setupDevicesWatcher();
    void updateAvdsList();
    IDevice::DeviceState getDeviceState(const QString &serial, IDevice::MachineType type) const;
    void updateDeviceState(const ProjectExplorer::IDevice::ConstPtr &device);

    void startAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);
    void eraseAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);
    void setupWifiForDevice(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);

    void setEmulatorArguments(QWidget *parent = nullptr);

    QString getRunningAvdsSerialNumber(const QString &name) const;

private:
    AndroidDeviceManager(QObject *parent = nullptr);
    ~AndroidDeviceManager();
    void HandleDevicesListChange(const QString &serialNumber);
    void HandleAvdsListChange();

    QString emulatorName(const QString &serialNumber) const;

    QFutureWatcher<AndroidDeviceInfoList> m_avdsFutureWatcher;
    std::unique_ptr<Utils::Process> m_removeAvdProcess;
    QFileSystemWatcher m_avdFileSystemWatcher;
    std::unique_ptr<Utils::Process> m_adbDeviceWatcherProcess;
    AndroidConfig &m_androidConfig;
    AndroidAvdManager m_avdManager;

    friend class AndroidPluginPrivate;
};

} // namespace Internal
} // namespace Android
