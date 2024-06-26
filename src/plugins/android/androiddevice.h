// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"
#include "androiddeviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <solutions/tasking/tasktreerunner.h>

#include <QSettings>

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

    void startAvd();

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
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

namespace AndroidDeviceManager {

void setupDevicesWatcher();
void updateAvdList();
Utils::expected_str<void> createAvd(const CreateAvdInfo &info, bool force);

} // namespace AndroidDeviceManager

void setupAndroidDevice();
void setupAndroidDeviceManager(QObject *guard);

} // Android::Internal
