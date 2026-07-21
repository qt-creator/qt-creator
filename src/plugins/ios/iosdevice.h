// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "deviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QtTaskTree/QMappedTaskTreeRunner>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVersionNumber>

#include <memory>
#include <optional>

namespace Ios {

namespace Internal {
class IosConfigurations;
class IosDeviceManager;

class IosDevice final : public ProjectExplorer::IDevice
{
public:
    using ConstPtr = std::shared_ptr<const IosDevice>;
    using Ptr = std::shared_ptr<IosDevice>;

    enum class Handler { IosTool, DeviceCtl };

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;

    QString uniqueDeviceID() const;
    const IosDeviceInfo &iosDeviceInformation() const;
    Handler handler() const;

    static QString name();

    static IosDevice::Ptr make() { return IosDevice::Ptr(new IosDevice()); }
    static IosDevice::Ptr make(const QString &uid) { return IosDevice::Ptr(new IosDevice(uid)); }

private:
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

    QtTaskTree::ExecutableItem portsGatheringRecipe(
        const QtTaskTree::Storage<Utils::PortsOutputData> &output) const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;

    friend class IosDeviceFactory;
    friend class Ios::Internal::IosDeviceManager;
    IosDevice();
    IosDevice(const QString &uid);

    enum CtorHelper {};
    IosDevice(CtorHelper);

    IosDeviceInfo m_extraInfo;
    Handler m_handler = Handler::IosTool;
    bool m_ignoreDevice = false;
};

class IosDeviceManager : public QObject
{
public:
    static IosDeviceManager *instance();

    void updateAvailableDevices(const QStringList &devices);
    void deviceConnected(const QString &uid, const QString &name = QString());
    void deviceDisconnected(const QString &uid);
    friend class IosConfigurations;
    void updateInfo(const QString &devId);
    void deviceInfo(const QString &deviceId,
                    IosDevice::Handler handler,
                    const IosDeviceInfo &info);
    void monitorAvailableDevices();

    static bool isDeviceCtlOutputSupported();
    static bool isDeviceCtlDebugSupported();

private:
    void updateUserModeDevices();
    IosDeviceManager(QObject *parent = nullptr);
    QtTaskTree::QMappedTaskTreeRunner<QString> m_updatesRunner; // deviceid->task
    QTimer m_userModeDevicesTimer;
    std::optional<QVersionNumber> m_deviceCtlVersion;
};

void setupIosDevice();

} // namespace Internal
} // namespace Ios
