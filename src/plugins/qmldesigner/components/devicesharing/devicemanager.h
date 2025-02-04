// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprojectmanager/qmlprojectexporter/resourcegenerator.h>

#include "deviceinfo.h"

#include <QPointer>

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE
namespace QmlDesigner::DeviceShare {
class Device;
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

    // project management functions
    void runProject(const QString &deviceId); // async
    void stopProject();                       // async

    DeviceManagerWidget *widget();

private:
    // Devices management
    QList<QSharedPointer<Device>> m_devices;
    QList<QSharedPointer<QUdpSocket>> m_udpSockets;

    // settings
    QString m_settingsPath;
    QString m_designStudioId;

    enum class ErrTypes {
        NoError,
        InternalError,
        ProjectPackingError,
        ProjectSendingError,
        ProjectStartError
    };
    enum class OpTypes { Stopped, Packing, Sending, Starting, Running };
    OpTypes m_currentState;
    QString m_currentDeviceId;
    QString m_currentQtKitVersion;
    bool m_processInterrupted;

    QmlProjectManager::QmlProjectExporter::ResourceGenerator m_resourceGenerator;

    QPointer<DeviceManagerWidget> m_widget;

private:
    // internal slots
    void initUdpDiscovery();
    void incomingDatagram();
    void incomingConnection();
    void readSettings();
    void writeSettings();
    void initDevice(const DeviceInfo &deviceInfo = DeviceInfo(),
                    const DeviceSettings &deviceSettings = DeviceSettings());

    // device signals
    void deviceInfoReceived(const QString &deviceId);

    QSharedPointer<Device> findDevice(const QString &deviceId) const;
    QString generateDeviceAlias() const;

    // internal functions
    void projectPacked(const Utils::FilePath &filePath);
    void handleError(const ErrTypes &errType,
                     const QString &deviceId,
                     const QString &error = QString());

signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void deviceOnline(const QString &deviceId);
    void deviceOffline(const QString &deviceId);
    void deviceActivated(const QString &deviceId);
    void deviceDeactivated(const QString &deviceId);
    void deviceAliasChanged(const QString &deviceId);

    void projectPacking(const QString &deviceId);
    void projectPackingError(const QString &deviceId, const QString &error);
    void projectSendingProgress(const QString &deviceId, const int percentage);
    void projectSendingError(const QString &deviceId, const QString &error);
    void projectStarting(const QString &deviceId);
    void projectStartingError(const QString &deviceId, const QString &error);
    void projectStarted(const QString &deviceId);
    void projectStopping(const QString &deviceId);
    void projectStopped(const QString &deviceId);
    void projectLogsReceived(const QString &deviceId, const QString &logs);

    void internalError(const QString &deviceId, const QString &error);
};

} // namespace QmlDesigner::DeviceShare
