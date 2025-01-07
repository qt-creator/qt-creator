// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTimer>
#include <QWebSocket>

#include "deviceinfo.h"

namespace QmlDesigner::DeviceShare {

class Device : public QObject
{
    Q_OBJECT
public:
    Device(const DeviceInfo &deviceInfo = {},
           const DeviceSettings &deviceSettings = {},
           QObject *parent = nullptr);
    ~Device();

    // device management
    DeviceInfo deviceInfo() const;
    DeviceSettings deviceSettings() const;
    void setDeviceInfo(const DeviceInfo &deviceInfo);
    void setDeviceSettings(const DeviceSettings &deviceSettings);

    // device communication
    bool sendDesignStudioReady(const QString &uuid);
    bool sendProjectData(const QByteArray &data);
    bool sendProjectStopped();

    // socket
    bool isConnected() const;
    void reconnect();

private slots:
    void processTextMessage(const QString &data);

private:
    DeviceInfo m_deviceInfo;
    DeviceSettings m_deviceSettings;

    QScopedPointer<QWebSocket> m_socket;
    bool m_socketWasConnected;

    QTimer m_reconnectTimer;
    QTimer m_pingTimer;
    QTimer m_pongTimer;

    int m_lastProjectSize;
    int m_lastProjectSentSize;

    static constexpr int m_reconnectTimeout = 5000;
    static constexpr int m_pingTimeout = 10000;
    static constexpr int m_pongTimeout = 30000;

    void initPingPong();
    bool sendProjectNotification(const int &projectSize);
    bool sendTextMessage(const QLatin1String &dataType, const QJsonValue &data = QJsonValue());
    bool sendBinaryMessage(const QByteArray &data);

signals:
    void connected(const QString &deviceId);
    void disconnected(const QString &deviceId);
    void deviceInfoReady(const QString &deviceIp, const QString &deviceId);
    void projectSendingProgress(const QString &deviceId, const int progress);
    void projectStarted(const QString &deviceId);
    void projectStopped(const QString &deviceId);
    void projectLogsReceived(const QString &deviceId, const QString &logs);
};
} // namespace QmlDesigner::DeviceShare
