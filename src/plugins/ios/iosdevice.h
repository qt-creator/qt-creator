// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iostoolhandler.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <solutions/tasking/tasktree.h>

#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QVersionNumber>

#include <optional>
#include <unordered_map>

namespace Ios {
class IosConfigurations;

namespace Internal {
class IosDeviceManager;

class IosDevice final : public ProjectExplorer::IDevice
{
public:
    using Dict = QMap<QString, QString>;
    using ConstPtr = std::shared_ptr<const IosDevice>;
    using Ptr = std::shared_ptr<IosDevice>;

    enum class Handler { IosTool, DeviceCtl };

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;

    QString deviceName() const;
    QString uniqueDeviceID() const;
    QString uniqueInternalDeviceId() const;
    QString osVersion() const;
    QString productType() const;
    QString cpuArchitecture() const;
    Handler handler() const;

    static QString name();

    static IosDevice::Ptr make() { return IosDevice::Ptr(new IosDevice()); }
    static IosDevice::Ptr make(const QString &uid) { return IosDevice::Ptr(new IosDevice(uid)); }

private:
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

    Tasking::ExecutableItem portsGatheringRecipe(
        const Tasking::Storage<Utils::PortsOutputData> &output) const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;

    friend class IosDeviceFactory;
    friend class Ios::Internal::IosDeviceManager;
    IosDevice();
    IosDevice(const QString &uid);

    enum CtorHelper {};
    IosDevice(CtorHelper);

    Dict m_extraInfo;
    Handler m_handler = Handler::IosTool;
    bool m_ignoreDevice = false;
};

class IosDeviceManager : public QObject
{
public:
    using TranslationMap = QHash<QString, QString>;

    static TranslationMap translationMap();
    static IosDeviceManager *instance();

    void updateAvailableDevices(const QStringList &devices);
    void deviceConnected(const QString &uid, const QString &name = QString());
    void deviceDisconnected(const QString &uid);
    friend class IosConfigurations;
    void updateInfo(const QString &devId);
    void deviceInfo(const QString &deviceId,
                    IosDevice::Handler handler,
                    const Ios::IosToolHandler::Dict &info);
    void monitorAvailableDevices();

    static bool isDeviceCtlOutputSupported();
    static bool isDeviceCtlDebugSupported();

private:
    void updateUserModeDevices();
    IosDeviceManager(QObject *parent = nullptr);
    std::unordered_map<QString, std::unique_ptr<Tasking::TaskTree>> m_updateTasks; // deviceid->task
    QTimer m_userModeDevicesTimer;
    QStringList m_userModeDeviceIds;
    QPointer<QMessageBox> m_devModeDialog;
    std::optional<QVersionNumber> m_deviceCtlVersion;
};

void setupIosDevice();

} // namespace Internal
} // namespace Ios
