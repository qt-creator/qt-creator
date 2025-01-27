// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "device.h"

#include <QJsonDocument>
#include <QLatin1String>

#include "websocketmock.h"

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
constexpr auto projectReceivingProgress = "projectReceivingProgress"_L1;
constexpr auto projectStarting = "projectStarting"_L1;
constexpr auto projectRunning = "projectRunning"_L1;
constexpr auto projectStopped = "projectStopped"_L1;
constexpr auto projectLogs = "projectLogs"_L1;
}; // namespace PackageFromDevice

Device::Device(const QString designStudioId,
               const DeviceInfo &deviceInfo,
               const DeviceSettings &deviceSettings,
               QObject *parent)
    : QObject(parent)
    , m_deviceInfo(deviceInfo)
    , m_deviceSettings(deviceSettings)
    , m_socket(nullptr)
    , m_socketWasConnected(false)
    , m_socketManuallyClosed(false)
    , m_designStudioId(designStudioId)
{
    qCDebug(deviceSharePluginLog) << "initial device info:" << m_deviceInfo;

    m_socket.reset(new QWebSocket());
    m_socket->setOutgoingFrameSize(128000);
    connect(m_socket.data(), &QWebSocket::textMessageReceived, this, &Device::processTextMessage);
    connect(m_socket.data(), &QWebSocket::disconnected, this, [this]() {
        if (m_socketManuallyClosed) {
            m_socketManuallyClosed = false;
            return;
        }

        m_reconnectTimer.start(m_reconnectTimeout);
        if (!m_socketWasConnected)
            return;

        m_socketWasConnected = false;
        stopPingPong();
        emit disconnected(m_deviceSettings.deviceId());
    });
    connect(m_socket.data(), &QWebSocket::connected, this, [this]() {
        m_socketWasConnected = true;
        m_reconnectTimer.stop();
        restartPingPong();
        sendDesignStudioReady();
        emit connected(m_deviceSettings.deviceId());
    });

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() { reconnect(); });

    m_sendTimer.setSingleShot(true);
    m_sendTimer.setInterval(10);
    connect(&m_sendTimer, &QTimer::timeout, this, &Device::sendProjectDataInternal, Qt::UniqueConnection);

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
    m_pingTimer.setInterval(m_pingTimeout);
    m_pongTimer.setInterval(m_pongTimeout);
    m_pongTimer.setSingleShot(true);
    m_pingTimer.setSingleShot(true);

    connect(&m_pingTimer, &QTimer::timeout, this, [this]() {
        m_socket->ping();
        m_pongTimer.start();
    });

    connect(m_socket.data(),
            &QWebSocket::pong,
            this,
            [this](quint64 elapsedTime, [[maybe_unused]] const QByteArray &payload) {
                if (elapsedTime > 1000)
                    qCWarning(deviceSharePluginLog)
                        << "Device pong is too slow:" << m_deviceSettings.alias() << "("
                        << m_deviceSettings.deviceId() << ") in" << elapsedTime
                        << "ms. Network issue?";
                else if (elapsedTime > 500)
                    qCWarning(deviceSharePluginLog)
                        << "Device pong is slow:" << m_deviceSettings.alias() << "("
                        << m_deviceSettings.deviceId() << ") in" << elapsedTime << "ms";

                m_pongTimer.stop();
                m_pingTimer.start();
            });

    connect(&m_pongTimer, &QTimer::timeout, this, [this]() {
        qCWarning(deviceSharePluginLog)
            << "Device" << m_deviceSettings.deviceId() << "is not responding. Closing connection.";
        m_socket->close();
        m_socket->abort();
    });
}

void Device::stopPingPong()
{
    m_pingTimer.stop();
    m_pongTimer.stop();
}

void Device::restartPingPong()
{
    m_pingTimer.start();
}

void Device::reconnect(const QString &closeMessage)
{
    if (m_socket && m_socket->isValid() && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close(QWebSocketProtocol::CloseCodeNormal, closeMessage);
        m_socketManuallyClosed = true;
    }

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
    QString oldIp = m_deviceSettings.ipAddress();
    m_deviceSettings = deviceSettings;
    if (oldIp != m_deviceSettings.ipAddress())
        reconnect();
}

bool Device::sendDesignStudioReady()
{
    QJsonObject data;
    data["designStudioID"] = m_designStudioId;
    data["commVersion"] = 1;
    return sendTextMessage(PackageToDevice::designStudioReady, data);
}

bool Device::sendProjectNotification(const int &projectSize, const QString &qtVersion)
{
    QJsonObject projectInfo;
    projectInfo["projectSize"] = projectSize;
    projectInfo["qtVersion"] = qtVersion;

    return sendTextMessage(PackageToDevice::projectData, projectInfo);
}

bool Device::sendProjectData(const QByteArray &data, const QString &qtVersion)
{
    if (!isConnected())
        return false;

    sendProjectNotification(data.size(), qtVersion);

    m_sendProject = true;
    m_projectData = data;
    m_totalSentSize = 0;
    m_lastSentProgress = 0;

    m_sendTimer.start();
    stopPingPong();

    return true;
}

void Device::sendProjectDataInternal()
{
    if (!isConnected())
        return;

    if (!m_sendProject) {
        sendTextMessage(PackageToDevice::stopRunningProject);
        restartPingPong();
        return;
    }

    const int chunkSize = 1024 * 50; // 50KB

    const QByteArray chunk = m_projectData.mid(m_totalSentSize, chunkSize);
    m_socket->sendBinaryMessage(chunk);
    m_socket->flush();

    m_totalSentSize += chunk.size();
    if (m_totalSentSize < m_projectData.size())
        m_sendTimer.start();
    else
        restartPingPong();
}

bool Device::sendProjectStopped()
{
    return sendTextMessage(PackageToDevice::stopRunningProject);
}

bool Device::isConnected() const
{
    return m_socket ? m_socket->state() == QAbstractSocket::ConnectedState : false;
}

bool Device::sendTextMessage(const QLatin1String &dataType, const QJsonValue &data)
{
    if (!isConnected())
        return false;

    QJsonObject message;
    message["dataType"] = dataType;
    message["data"] = data;
    const QString jsonMessage = QString::fromLatin1(
        QJsonDocument(message).toJson(QJsonDocument::Compact));
    m_socket->sendTextMessage(jsonMessage);

    return true;
}

void Device::abortProjectTransmission()
{
    if (!isConnected())
        return;

    m_sendProject = false;
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

        const QString os = m_deviceInfo.os();
        if (os.contains("Android", Qt::CaseInsensitive))
            m_deviceInfo.setOs("Android");
        else if (os.contains("iOS", Qt::CaseInsensitive))
            m_deviceInfo.setOs("iOS");
        else if (os.contains("iPadOS", Qt::CaseInsensitive))
            m_deviceInfo.setOs("iPadOS");
        else if (os.contains("Windows", Qt::CaseInsensitive))
            m_deviceInfo.setOs("Windows");
        else if (os.contains("Linux", Qt::CaseInsensitive))
            m_deviceInfo.setOs("Linux");
        else if (os.contains("Mac", Qt::CaseInsensitive))
            m_deviceInfo.setOs("macOS");

        emit deviceInfoReady(m_deviceSettings.deviceId());
    } else if (dataType == PackageFromDevice::projectRunning) {
        qCDebug(deviceSharePluginLog) << "Project started on device" << m_deviceSettings.deviceId();
        emit projectStarted(m_deviceSettings.deviceId());
    } else if (dataType == PackageFromDevice::projectStopped) {
        qCDebug(deviceSharePluginLog) << "Project stopped on device" << m_deviceSettings.deviceId();
        emit projectStopped(m_deviceSettings.deviceId());
    } else if (dataType == PackageFromDevice::projectLogs) {
        const QString logs = jsonObj.value("data").toString();
        qCDebug(deviceSharePluginLog) << "Device Log:" << m_deviceSettings.deviceId() << logs;
        emit projectLogsReceived(m_deviceSettings.deviceId(), logs);
    } else if (dataType == PackageFromDevice::projectStarting) {
        qCDebug(deviceSharePluginLog) << "Project starting on device" << m_deviceSettings.deviceId();
        emit projectStarting(m_deviceSettings.deviceId());
    } else if (dataType == PackageFromDevice::projectReceivingProgress) {
        const int progress = jsonObj.value("data").toInt();
        emit projectSendingProgress(m_deviceSettings.deviceId(), progress);
    } else {
        qCDebug(deviceSharePluginLog) << "Invalid JSON message:" << jsonObj;
    }
}

} // namespace QmlDesigner::DeviceShare
