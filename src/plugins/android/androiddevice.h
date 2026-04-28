// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"
#include "androiddeviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QtTaskTree/QTaskTree>

#include <QSettings>

namespace Android::Internal {

class AndroidDevice final : public ProjectExplorer::IDevice
{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<AndroidDevice>;
    using ConstPtr = std::shared_ptr<const AndroidDevice>;

    // Returns the device as a ready-to-use Android device, or null if it is not
    // an Android device or not currently ready.
    static ConstPtr asReady(const ProjectExplorer::IDeviceConstPtr &device);

    AndroidDevice();
    ~AndroidDevice() override;

    static IDevice::Ptr create();
    static AndroidDeviceInfo androidDeviceInfoFromDevice(const IDevice::ConstPtr &dev);

    static Utils::Id idFromDeviceInfo(const AndroidDeviceInfo &info);
    static Utils::Id idFromAvdInfo(const CreateAvdInfo &info);

    QStringList supportedAbis() const;
    bool canSupportAbis(const QStringList &abis) const;

    bool canHandleDeployments() const;

    QString serialNumber() const;
    QString avdName() const;
    int sdkLevel() const;

    // "<display name> (<serial>)", or just the display name when no serial is cached
    QString displayNameWithSerial() const;

    Utils::FilePath avdPath() const;
    void setAvdPath(const Utils::FilePath &path);

    QString deviceTypeName() const;
    QString androidVersion() const;

    // AVD specific
    QString skinName() const;
    QString androidTargetName() const;
    QString sdcardSize() const;
    QString openGLStatus() const;

    void startAvd();

    void updateDeviceFileAccess();
    void addActionsIfNotFound();
    void updateSerialNumber(const QString &serial);

protected:
    void fromMap(const Utils::Store &map) final;

private:
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    QtTaskTree::ExecutableItem signalOperationRecipe(
        const ProjectExplorer::SignalOperationData &data,
        const QtTaskTree::Storage<Utils::Result<>> &resultStorage) const final;
    QUrl toolControlChannel(const ControlChannelHint &) const override;
    QtTaskTree::ExecutableItem portsGatheringRecipe(
        const QtTaskTree::Storage<Utils::PortsOutputData> &output) const override;
    Utils::ProcessInterface *createProcessInterface() const final;

    QSettings *avdSettings() const;
    void initAvdSettings();

    class AndroidDevicePrivate *d;
};

void setupDevicesWatcher();
void updateAvdList();
QtTaskTree::Group createAvdRecipe(const QtTaskTree::Storage<std::optional<QString>> &errorStorage,
                               const CreateAvdInfo &info, bool force);

void setupAndroidDevice();
void setupAndroidDeviceManager();

} // Android::Internal
