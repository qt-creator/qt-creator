// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iostoolhandler.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QMessageBox>
#include <QPointer>
#include <QTimer>

namespace Ios {
class IosConfigurations;

namespace Internal {
class IosDeviceManager;

class IosDevice final : public ProjectExplorer::IDevice
{
public:
    using Dict = QMap<QString, QString>;
    using ConstPtr = QSharedPointer<const IosDevice>;
    using Ptr = QSharedPointer<IosDevice>;

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;

    QString deviceName() const;
    QString uniqueDeviceID() const;
    QString uniqueInternalDeviceId() const;
    QString osVersion() const;
    QString cpuArchitecture() const;
    Utils::Port nextPort() const;

    static QString name();

protected:
    void fromMap(const Utils::Store &map) final;
    Utils::Store toMap() const final;

    friend class IosDeviceFactory;
    friend class Ios::Internal::IosDeviceManager;
    IosDevice();
    IosDevice(const QString &uid);

    enum CtorHelper {};
    IosDevice(CtorHelper);

    Dict m_extraInfo;
    bool m_ignoreDevice = false;
    mutable quint16 m_lastPort;
};

class IosDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    IosDeviceFactory();

    bool canRestore(const Utils::Store &map) const override;
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
    void deviceInfo(Ios::IosToolHandler *gatherer, const QString &deviceId,
                    const Ios::IosToolHandler::Dict &info);
    void infoGathererFinished(Ios::IosToolHandler *gatherer);
    void monitorAvailableDevices();

private:
    void updateUserModeDevices();
    IosDeviceManager(QObject *parent = nullptr);
    QTimer m_userModeDevicesTimer;
    QStringList m_userModeDeviceIds;
    QPointer<QMessageBox> m_devModeDialog;
};

} // namespace Internal
} // namespace Ios
