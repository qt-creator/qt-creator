// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTimer>

#include <atomic>

#include "deviceinfo.h"

class QWebSocket;
namespace QmlDesigner::DeviceShare {

class Device : public QObject
{
    Q_OBJECT
public:
    Device(const QString designStudioId,
           const DeviceInfo &deviceInfo = {},
           const DeviceSettings &deviceSettings = {},
           QObject *parent = nullptr);
    ~Device();

    // device management
    DeviceInfo deviceInfo() const;
    DeviceSettings deviceSettings() const;
    void setDeviceInfo(const DeviceInfo &deviceInfo);
    void setDeviceSettings(const DeviceSettings &deviceSettings);

    // device communication
    bool sendProjectData(const QByteArray &data, const QString &qtVersion);
    bool sendProjectStopped();
    void abortProjectTransmission();

    // socket
    bool isConnected() const;
    void reconnect(const QString &closeMessage = QString());

private slots:
    void processTextMessage(const QString &data);

private:
    DeviceInfo m_deviceInfo;
    DeviceSettings m_deviceSettings;

    QScopedPointer<QWebSocket> m_socket;
    bool m_socketWasConnected;
    bool m_socketManuallyClosed;

    QTimer m_reconnectTimer;
    QTimer m_pingTimer;
    QTimer m_pongTimer;
    QTimer m_sendTimer;

    std::atomic<bool> m_sendProject;
    QByteArray m_projectData;
    int m_totalSentSize;
    int m_lastSentProgress;

    const QString m_designStudioId;

    static constexpr int m_reconnectTimeout = 5000;
    static constexpr int m_pingTimeout = 10000;
    static constexpr int m_pongTimeout = 30000;

    void initPingPong();
    void stopPingPong();
    void restartPingPong();
    void sendProjectDataInternal();
    bool sendDesignStudioReady();
    bool sendProjectNotification(const int &projectSize, const QString &qtVersion);
    bool sendTextMessage(const QLatin1String &dataType, const QJsonValue &data = QJsonValue());
    bool sendBinaryMessage(const QByteArray &data);

signals:
    void connected(const QString &deviceId);
    void disconnected(const QString &deviceId);
    void deviceInfoReady(const QString &deviceId);
    void projectSendingProgress(const QString &deviceId, const int progress);
    void projectStarting(const QString &deviceId);
    void projectStarted(const QString &deviceId);
    void projectStopped(const QString &deviceId);
    void projectLogsReceived(const QString &deviceId, const QString &logs);
};
} // namespace QmlDesigner::DeviceShare
