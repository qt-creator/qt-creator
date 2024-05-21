// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"
#include "androiddeviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/guard.h>

#include <QFileSystemWatcher>
#include <QSettings>

namespace Utils { class Process; }

namespace Android::Internal {

class AndroidDevice final : public ProjectExplorer::IDevice
{
public:
    AndroidDevice();

    static IDevice::Ptr create();
    static AndroidDeviceInfo androidDeviceInfoFromIDevice(const IDevice *dev);

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

class AndroidDeviceManager : public QObject
{
public:
    static AndroidDeviceManager *instance();
    void setupDevicesWatcher();
    void updateAvdList();
    IDevice::DeviceState getDeviceState(const QString &serial, IDevice::MachineType type) const;
    void updateDeviceState(const ProjectExplorer::IDevice::ConstPtr &device);

    Utils::expected_str<void> createAvd(const CreateAvdInfo &info, bool force);
    void startAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);
    void eraseAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);
    void setupWifiForDevice(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);

    void setEmulatorArguments(QWidget *parent = nullptr);

    QString getRunningAvdsSerialNumber(const QString &name) const;

    static ProjectExplorer::IDevice::Ptr createDeviceFromInfo(const CreateAvdInfo &info);

private:
    explicit AndroidDeviceManager(QObject *parent);
    ~AndroidDeviceManager();

    void handleDevicesListChange(const QString &serialNumber);
    void handleAvdListChange(const AndroidDeviceInfoList &avdList);

    QString emulatorName(const QString &serialNumber) const;

    Tasking::Group m_avdListRecipe;
    Tasking::TaskTreeRunner m_avdListRunner;
    std::unique_ptr<Utils::Process> m_removeAvdProcess;
    QFileSystemWatcher m_avdFileSystemWatcher;
    Utils::Guard m_avdPathGuard;
    std::unique_ptr<Utils::Process> m_adbDeviceWatcherProcess;

    friend void setupAndroidDeviceManager(QObject *guard);
};

void setupAndroidDevice();
void setupAndroidDeviceManager(QObject *guard);

} // Android::Internal
