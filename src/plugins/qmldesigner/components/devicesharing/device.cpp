// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "device.h"

#include <QJsonDocument>
#include <QLatin1String>

namespace QmlDesigner::DeviceShare {

// Below are the constants that are used in the communication between the Design Studio and the device.
namespace PackageToDevice {
using namespace Qt::Literals;
constexpr auto designStudioReady = "designStudioReady"_L1;
constexpr auto projectData = "projectData"_L1;
constexpr auto stopRunningProject = "stopRunningProject"_L1;
}; // namespace PackageToDevice

namespace PackageFromDevice {
using namespace Qt::Literals;
constexpr auto deviceInfo = "deviceInfo"_L1;
constexpr auto projectRunning = "projectRunning"_L1;
constexpr auto projectStopped = "projectStopped"_L1;
constexpr auto projectLogs = "projectLogs"_L1;
}; // namespace PackageFromDevice

Device::Device(const DeviceInfo &deviceInfo, const DeviceSettings &deviceSettings, QObject *parent)
    : QObject(parent)
    , m_deviceInfo(deviceInfo)
    , m_deviceSettings(deviceSettings)
    , m_socket(nullptr)
    , m_socketWasConnected(false)
{
    qCDebug(deviceSharePluginLog) << "initial device info:" << m_deviceInfo;

    m_socket.reset(new QWebSocket());
    connect(m_socket.data(), &QWebSocket::textMessageReceived, this, &Device::processTextMessage);
    connect(m_socket.data(), &QWebSocket::disconnected, this, [this]() {
        m_reconnectTimer.start();
        if (!m_socketWasConnected)
            return;

        m_socketWasConnected = false;
        m_pingTimer.stop();
        m_pongTimer.stop();
        emit disconnected(m_deviceInfo.deviceId());
    });
    connect(m_socket.data(), &QWebSocket::connected, this, [this]() {
        m_socketWasConnected = true;
        m_reconnectTimer.stop();
        m_pingTimer.start(15000);
        sendDesignStudioReady(m_deviceInfo.deviceId());
        emit connected(m_deviceInfo.deviceId());
    });

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(5000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &Device::reconnect);

    initPingPong();
    reconnect();
}

Device::~Device()
{
    m_socket->close();
    m_socket.reset();
}

void Device::initPingPong()
{
    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        m_socket->ping();
        m_pongTimer.start(15000);
    });

    connect(m_socket.data(),
            &QWebSocket::pong,
            this,
            [this](quint64 elapsedTime, [[maybe_unused]] const QByteArray &payload) {
                qCDebug(deviceSharePluginLog)
                    << "Pong received from Device" << m_deviceInfo.deviceId() << "in" << elapsedTime
                    << "ms";
                m_pongTimer.stop();
            });

    connect(&m_pongTimer, &QTimer::timeout, this, [this]() {
        qCDebug(deviceSharePluginLog)
            << "Device" << m_deviceInfo.deviceId() << "is not responding. Closing connection.";
        m_socket->close();
    });

    m_pongTimer.setSingleShot(true);
}

void Device::reconnect()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->close();

    QUrl url(QStringLiteral("ws://%1:%2").arg(m_deviceSettings.ipAddress()).arg(40000));
    m_socket->open(url);
}

DeviceInfo Device::deviceInfo() const
{
    return m_deviceInfo;
}

void Device::setDeviceInfo(const DeviceInfo &deviceInfo)
{
    m_deviceInfo = deviceInfo;
}

DeviceSettings Device::deviceSettings() const
{
    return m_deviceSettings;
}

void Device::setDeviceSettings(const DeviceSettings &deviceSettings)
{
    m_deviceSettings = deviceSettings;
    reconnect();
}

void Device::sendDesignStudioReady(const QString &uuid) {
    sendTextMessage(PackageToDevice::designStudioReady, uuid);
}

void Device::sendProjectNotification()
{
    sendTextMessage(PackageToDevice::projectData);
}

void Device::sendProjectData(const QByteArray &data)
{
    sendBinaryMessage(data);
}

void Device::sendProjectStopped()
{
    sendTextMessage(PackageToDevice::stopRunningProject);
}

bool Device::isConnected() const
{
    return m_socket ? m_socket->state() == QAbstractSocket::ConnectedState : false;
}

void Device::sendTextMessage(const QLatin1String &dataType, const QJsonValue &data)
{
    if (!isConnected())
        return;

    QJsonObject message;
    message["dataType"] = dataType;
    message["data"] = data;
    const QString jsonMessage = QString::fromLatin1(
        QJsonDocument(message).toJson(QJsonDocument::Compact));
    m_socket->sendTextMessage(jsonMessage);
}

void Device::sendBinaryMessage(const QByteArray &data)
{
    if (!isConnected())
        return;

    m_socket->sendBinaryMessage(data);
}

void Device::processTextMessage(const QString &data)
{
    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toLatin1(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qCDebug(deviceSharePluginLog)
            << "Failed to parse JSON message:" << jsonError.errorString() << data;
        return;
    }

    const QJsonObject jsonObj = jsonDoc.object();
    const QString dataType = jsonObj.value("dataType").toString();
    if (dataType == PackageFromDevice::deviceInfo) {
        QJsonObject deviceInfo = jsonObj.value("data").toObject();
        m_deviceInfo.setJsonObject(deviceInfo);
        emit deviceInfoReady(m_deviceInfo.deviceId(), m_deviceInfo);
    } else if (dataType == PackageFromDevice::projectRunning) {
        emit projectStarted(m_deviceInfo.deviceId());
    } else if (dataType == PackageFromDevice::projectStopped) {
        emit projectStopped(m_deviceInfo.deviceId());
    } else if (dataType == PackageFromDevice::projectLogs) {
        emit projectLogsReceived(m_deviceInfo.deviceId(), jsonObj.value("data").toString());
    } else {
        qCDebug(deviceSharePluginLog) << "Invalid JSON message:" << jsonObj;
    }
}

} // namespace QmlDesigner::DeviceShare
