// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointer>
#include <QUdpSocket>
#include <QWebSocketServer>

#include "device.h"

namespace QmlDesigner::DeviceShare {

class DeviceManagerWidget;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // Getters
    QList<QSharedPointer<Device>> devices() const;

    std::optional<DeviceInfo> deviceInfo(const QString &deviceId) const;
    std::optional<DeviceSettings> deviceSettings(const QString &deviceId) const;

    std::optional<bool> deviceIsConnected(const QString &deviceId) const;

    // Device management functions
    void setDeviceAlias(const QString &deviceId, const QString &alias);
    void setDeviceActive(const QString &deviceId, const bool active);
    void setDeviceIP(const QString &deviceId, const QString &ip);

    bool addDevice(const QString &ip);
    void removeDevice(const QString &deviceId);
    void removeDeviceAt(int index);
    bool sendProjectFile(const QString &deviceId, const QString &projectFile);
    bool stopRunningProject(const QString &deviceId);

    DeviceManagerWidget *widget();

private:
    // Devices management
    QList<QSharedPointer<Device>> m_devices;
    QList<QSharedPointer<QUdpSocket>> m_udpSockets;

    // settings
    QString m_settingsPath;
    QString m_uuid;

    QPointer<DeviceManagerWidget> m_widget;

private:
    // internal slots
    void initUdpDiscovery();
    void incomingDatagram();
    void incomingConnection();
    void readSettings();
    void writeSettings();
    QSharedPointer<Device> initDevice(const DeviceInfo &deviceInfo = DeviceInfo(),
                                      const DeviceSettings &deviceSettings = DeviceSettings());

    // device signals
    void deviceInfoReceived(const QString &deviceIp, const QString &deviceId);
    void deviceDisconnected(const QString &deviceId);

    QSharedPointer<Device> findDevice(const QString &deviceId) const;
    QString generateDeviceAlias() const;

signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void deviceOnline(const QString &deviceId);
    void deviceOffline(const QString &deviceId);
    void deviceActivated(const QString &deviceId);
    void deviceDeactivated(const QString &deviceId);
    void deviceAliasChanged(const QString &deviceId);
    void projectStarted(const QString &deviceId);
    void projectStopped(const QString &deviceId);
    void projectLogsReceived(const QString &deviceId, const QString &logs);
};

} // namespace QmlDesigner::DeviceShare
