// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanager.h"
#include "device.h"
#include "devicemanagerwidget.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLatin1String>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>

#include <coreplugin/icore.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/target.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>
#include <qtsupport/qtkitaspect.h>

namespace QmlDesigner::DeviceShare {

namespace JsonKeys {
using namespace Qt::Literals;
constexpr auto devices = "devices"_L1;
constexpr auto designStudioId = "designStudioId"_L1;
constexpr auto deviceInfo = "deviceInfo"_L1;
constexpr auto deviceSettings = "deviceSettings"_L1;
} // namespace JsonKeys

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_currentState(OpTypes::Stopped)
    , m_processInterrupted(false)
{
    QFileInfo fileInfo(Core::ICore::settings()->fileName());
    m_settingsPath = fileInfo.absolutePath() + "/device_manager.json";
    readSettings();
    initUdpDiscovery();

    connect(&m_resourceGenerator,
            &QmlProjectManager::QmlProjectExporter::ResourceGenerator::errorOccurred,
            this,
            [this](const QString &error) {
                qCDebug(deviceSharePluginLog) << "ResourceGenerator error:" << error;
                handleError(ErrTypes::ProjectPackingError, m_currentDeviceId, error);
            });

    connect(&m_resourceGenerator,
            &QmlProjectManager::QmlProjectExporter::ResourceGenerator::qmlrcCreated,
            this,
            &DeviceManager::projectPacked);
}

DeviceManager::~DeviceManager() = default;

void DeviceManager::initUdpDiscovery()
{
    QSharedPointer<QUdpSocket> udpSocket = QSharedPointer<QUdpSocket>::create();
    bool retVal = udpSocket->bind(QHostAddress::AnyIPv4, 53452, QUdpSocket::ShareAddress);
    if (!retVal) {
        qCWarning(deviceSharePluginLog) << "UDP:: Failed to bind to UDP port 53452 on AnyIPv4"
                                        << ". Error:" << udpSocket->errorString();
        return;
    }

    connect(udpSocket.data(), &QUdpSocket::readyRead, this, &DeviceManager::incomingDatagram);

    qCDebug(deviceSharePluginLog) << "UDP:: Listening on AnyIPv4 port" << udpSocket->localPort();
    m_udpSockets.append(udpSocket);
}

void DeviceManager::incomingDatagram()
{
    const auto udpSocket = qobject_cast<QUdpSocket *>(sender());
    if (!udpSocket)
        return;

    while (udpSocket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = udpSocket->receiveDatagram();
        const QJsonDocument doc = QJsonDocument::fromJson(datagram.data());
        const QJsonObject message = doc.object();

        if (message["name"].toString() != "__qtuiviewer__") {
            continue;
        }

        const QString id = message["id"].toString();
        const QString ip = datagram.senderAddress().toString();

        bool found = false;
        for (const auto &device : m_devices) {
            if (device->deviceInfo().selfId() == id) {
                found = true;
                if (device->deviceSettings().ipAddress() != ip) {
                    qCDebug(deviceSharePluginLog)
                        << "Updating IP address for device" << id << "from"
                        << device->deviceSettings().ipAddress() << "to" << ip;
                    setDeviceIP(device->deviceSettings().deviceId(), ip);
                }
            }
        }

        if (!found)
            qCDebug(deviceSharePluginLog) << "Qt UI VIewer discovered at" << ip << "with id" << id;
    }
}

void DeviceManager::writeSettings()
{
    QJsonObject root;
    QJsonArray devices;
    for (const auto &device : m_devices) {
        QJsonObject deviceInfo;
        deviceInfo.insert(JsonKeys::deviceInfo, device->deviceInfo().jsonObject());
        deviceInfo.insert(JsonKeys::deviceSettings, device->deviceSettings().jsonObject());
        devices.append(deviceInfo);
    }

    root.insert(JsonKeys::devices, devices);
    root.insert(JsonKeys::designStudioId, m_designStudioId);

    QJsonDocument doc(root);
    QFile file(m_settingsPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(deviceSharePluginLog) << "Failed to open settings file" << file.fileName();
        return;
    }

    file.write(doc.toJson());
}

void DeviceManager::readSettings()
{
    QFile file(m_settingsPath);
    qCDebug(deviceSharePluginLog) << "Reading settings from" << file.fileName();
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(deviceSharePluginLog) << "Failed to open settings file" << file.fileName();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_designStudioId = doc.object()[JsonKeys::designStudioId].toString();
    if (m_designStudioId.isEmpty()) {
        m_designStudioId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        writeSettings();
    }

    QJsonArray devices = doc.object()[JsonKeys::devices].toArray();
    for (const QJsonValue &deviceInfoJson : devices) {
        DeviceInfo deviceInfo{deviceInfoJson.toObject()[JsonKeys::deviceInfo].toObject()};
        DeviceSettings deviceSettings{deviceInfoJson.toObject()[JsonKeys::deviceSettings].toObject()};
        initDevice(deviceInfo, deviceSettings);
    }
}

QList<QSharedPointer<Device>> DeviceManager::devices() const
{
    return m_devices;
}

QSharedPointer<Device> DeviceManager::findDevice(const QString &deviceId) const
{
    auto it = std::find_if(m_devices.begin(), m_devices.end(), [deviceId](const auto &device) {
        return device->deviceSettings().deviceId() == deviceId;
    });

    return it != m_devices.end() ? *it : nullptr;
}

std::optional<DeviceInfo> DeviceManager::deviceInfo(const QString &deviceId) const
{
    auto device = findDevice(deviceId);
    if (!device)
        return {};

    return device->deviceInfo();
}

std::optional<DeviceSettings> DeviceManager::deviceSettings(const QString &deviceId) const
{
    auto device = findDevice(deviceId);
    if (!device)
        return {};

    return device->deviceSettings();
}

std::optional<bool> DeviceManager::deviceIsConnected(const QString &deviceId) const
{
    auto device = findDevice(deviceId);
    if (!device)
        return {};

    return device->isConnected();
}

void DeviceManager::setDeviceAlias(const QString &deviceId, const QString &alias)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_SET_ALIAS);

    auto device = findDevice(deviceId);
    if (!device)
        return;

    auto deviceSettings = device->deviceSettings();

    if (deviceSettings.alias() == alias)
        return;

    deviceSettings.setAlias(alias);
    device->setDeviceSettings(deviceSettings);
    writeSettings();

    emit deviceAliasChanged(device->deviceInfo());
}

void DeviceManager::setDeviceActive(const QString &deviceId, const bool active)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_SET_ACTIVE);
    auto device = findDevice(deviceId);
    if (!device)
        return;

    auto deviceSettings = device->deviceSettings();
    deviceSettings.setActive(active);
    device->setDeviceSettings(deviceSettings);
    writeSettings();

    if (active)
        emit deviceActivated(deviceId);
    else
        emit deviceDeactivated(deviceId);
}

void DeviceManager::setDeviceIP(const QString &deviceId, const QString &ip)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_SET_DEVICE_IP);
    auto device = findDevice(deviceId);
    if (!device)
        return;

    auto deviceSettings = device->deviceSettings();
    deviceSettings.setIpAddress(ip);
    device->setDeviceSettings(deviceSettings);
    writeSettings();
}

QString DeviceManager::generateDeviceAlias() const
{
    QStringList m_currentAliases;
    for (const auto &device : m_devices)
        m_currentAliases.append(device->deviceSettings().alias());

    QString alias = "Device 0";
    int index = 0;
    while (m_currentAliases.contains(alias))
        alias = QString("Device %1").arg(++index);

    return alias;
}

bool DeviceManager::addDevice(const QString &ip)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_ADD_DEVICE);
    if (ip.isEmpty())
        return false;

    const auto trimmedIp = ip.trimmed();

    // check regex for xxx.xxx.xxx.xxx
    QRegularExpression ipRegex(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
    if (!ipRegex.match(trimmedIp).hasMatch()) {
        qCWarning(deviceSharePluginLog) << "Invalid IP address" << ip;
        return false;
    }

    for (const auto &device : m_devices) {
        if (device->deviceSettings().ipAddress() == trimmedIp) {
            qCWarning(deviceSharePluginLog) << "Device" << trimmedIp << "already exists";
            return false;
        }
    }

    DeviceSettings deviceSettings;
    deviceSettings.setIpAddress(trimmedIp);
    deviceSettings.setAlias(generateDeviceAlias());
    deviceSettings.setDeviceId(QUuid::createUuid().toString(QUuid::WithoutBraces));

    initDevice({}, deviceSettings);
    writeSettings();
    emit deviceAdded(deviceSettings.deviceId());

    return true;
}

void DeviceManager::initDevice(const DeviceInfo &deviceInfo, const DeviceSettings &deviceSettings)
{
    QSharedPointer<Device> device = QSharedPointer<Device>(new Device{m_designStudioId,
                                                                      deviceInfo,
                                                                      deviceSettings},
                                                           &QObject::deleteLater);
    QString deviceId = device->deviceSettings().deviceId();
    connect(device.data(), &Device::deviceInfoReady, this, &DeviceManager::deviceInfoReceived);
    connect(device.data(), &Device::disconnected, this, [this](const QString &deviceId) {
        qCDebug(deviceSharePluginLog) << "Device" << deviceId << "disconnected";
        emit deviceOffline(deviceId);
        handleError(ErrTypes::NoError, deviceId);
    });
    connect(device.data(), &Device::projectSendingProgress, this, &DeviceManager::projectSendingProgress);

    connect(device.data(), &Device::projectStarting, this, [this](const QString &deviceId) {
        m_currentState = OpTypes::Starting;
        emit projectStarting(deviceId);
    });

    connect(device.data(), &Device::projectStarted, this, [this](const QString &deviceId) {
        m_currentState = OpTypes::Running;
        emit projectStarted(deviceId);
    });

    connect(device.data(), &Device::projectStopped, this, [this](const QString &deviceId) {
        handleError(ErrTypes::NoError, deviceId);
    });
    connect(device.data(), &Device::projectLogsReceived, this, &DeviceManager::projectLogsReceived);

    m_devices.append(device);
}

void DeviceManager::deviceInfoReceived(const QString &deviceId)
{
    auto newDevice = findDevice(deviceId);
    const QString selfId = newDevice->deviceInfo().selfId();
    const QString deviceIp = newDevice->deviceSettings().ipAddress();

    auto oldDevIt = std::find_if(m_devices.begin(),
                                 m_devices.end(),
                                 [selfId, deviceIp](const auto &device) {
                                     return device->deviceInfo().selfId() == selfId
                                            && device->deviceSettings().ipAddress() != deviceIp;
                                 });

    // if there are 2 devices with the same ID but different IPs, remove the old one
    // aka: merge devices with the same ID
    if (oldDevIt != m_devices.end()) {
        auto oldDevice = *oldDevIt;
        const QString alias = oldDevice->deviceSettings().alias();
        m_devices.removeOne(*oldDevIt);
        DeviceSettings settings = newDevice->deviceSettings();
        settings.setAlias(alias);
        newDevice->setDeviceSettings(settings);
    }

    writeSettings();
    qCDebug(deviceSharePluginLog) << "Device" << deviceId << "is online";
    emit deviceOnline(deviceId);
}

void DeviceManager::removeDevice(const QString &deviceId)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_REMOVE_DEVICE);
    auto device = findDevice(deviceId);
    if (!device)
        return;

    m_devices.removeOne(device);
    writeSettings();
    emit deviceRemoved(deviceId);
}

void DeviceManager::removeDeviceAt(int index)
{
    if (index < 0 || index >= m_devices.size())
        return;

    QString deviceId = m_devices[index]->deviceSettings().deviceId();
    m_devices.removeAt(index);
    writeSettings();
    emit deviceRemoved(deviceId);
}

void DeviceManager::handleError(const ErrTypes &errType, const QString &deviceId, const QString &error)
{
    if (!m_processInterrupted) {
        if (errType != ErrTypes::NoError)
            qCWarning(deviceSharePluginLog) << "Handling error" << error << "for device" << deviceId;

        switch (errType) {
        case ErrTypes::InternalError:
            emit internalError(deviceId, error);
            break;
        case ErrTypes::ProjectPackingError:
            emit projectPackingError(deviceId, error);
            break;
        case ErrTypes::ProjectSendingError:
            emit projectSendingError(deviceId, error);
            break;
        case ErrTypes::ProjectStartError:
            emit projectStartingError(deviceId, error);
            break;
        case ErrTypes::NoError:
            break;
        }
    }

    m_processInterrupted = false;
    m_currentQtKitVersion.clear();
    m_currentDeviceId.clear();
    m_currentState = OpTypes::Stopped;
    emit projectStopped(deviceId);
}

void DeviceManager::runProject(const QString &deviceId)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_RUN_PROJECT);
    auto device = findDevice(deviceId);
    if (!device) {
        handleError(ErrTypes::InternalError, deviceId, "Device not found");
        return;
    }

    if (m_currentState != OpTypes::Stopped) {
        qCDebug(deviceSharePluginLog) << "Another operation is in progress";
        return;
    }

    m_currentQtKitVersion.clear();
    ProjectExplorer::Target *target = QmlDesignerPlugin::instance()->currentDesignDocument()->currentTarget();
    if (target && target->kit()) {
        if (QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(target->kit()))
            m_currentQtKitVersion = qtVer->qtVersion().toString();
    }

    m_currentState = OpTypes::Packing;
    m_currentDeviceId = deviceId;
    m_resourceGenerator.createQmlrcAsync(ProjectExplorer::ProjectManager::startupProject());
    emit projectPacking(deviceId);
    qCDebug(deviceSharePluginLog) << "Packing project for device" << deviceId;
}

void DeviceManager::projectPacked(const Utils::FilePath &filePath)
{
    qCDebug(deviceSharePluginLog) << "Project packed" << filePath.toUserOutput();

    // it is possible that the device was disconnected while the project was being packed
    if (m_currentDeviceId.isEmpty()) {
        qCDebug(deviceSharePluginLog) << "Device disconnected while project was being packed";
        return;
    }

    qCDebug(deviceSharePluginLog) << "Sending project file to device" << m_currentDeviceId;
    QFile file(filePath.toFSPathString());

    if (!file.open(QIODevice::ReadOnly)) {
        handleError(ErrTypes::ProjectSendingError, m_currentDeviceId, "Failed to open project file");
        return;
    }

    ProjectExplorer::Target *target = QmlDesignerPlugin::instance()->currentDesignDocument()->currentTarget();
    if (target && target->kit()) {
        if (QtSupport::QtVersion *qtVer = QtSupport::QtKitAspect::qtVersion(target->kit()))
            m_currentQtKitVersion = qtVer->qtVersion().toString();
    }

    auto device = findDevice(m_currentDeviceId);
    if (!device) {
        handleError(ErrTypes::InternalError, m_currentDeviceId, "Device not found");
        return;
    }

    if (!device->sendProjectData(file.readAll(), m_currentQtKitVersion)) {
        handleError(ErrTypes::ProjectSendingError, m_currentDeviceId, "Failed to send project file");
        return;
    }

    m_currentState = OpTypes::Sending;
    emit projectSendingProgress(m_currentDeviceId, 0);
}

void DeviceManager::stopProject()
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DEVICE_MANAGER_ANDROID_STOP_PROJECT);
    auto device = findDevice(m_currentDeviceId);
    if (!device) {
        handleError(ErrTypes::InternalError, m_currentDeviceId, "Device not found");
        return;
    }

    m_processInterrupted = true;
    switch (m_currentState) {
    case OpTypes::Stopped:
        qCWarning(deviceSharePluginLog) << "No project is running";
        return;
    case OpTypes::Packing:
        qCDebug(deviceSharePluginLog) << "Canceling project packing";
        m_resourceGenerator.cancel();
        break;
    case OpTypes::Sending:
        qCDebug(deviceSharePluginLog) << "Cancelling project sending";
        device->abortProjectTransmission();
        break;
    case OpTypes::Starting:
    case OpTypes::Running:
        qCDebug(deviceSharePluginLog) << "Stopping project on device" << m_currentDeviceId;
        device->sendProjectStopped();
        break;
    }

    emit projectStopping(m_currentDeviceId);
}

DeviceManagerWidget *DeviceManager::widget()
{
    if (!m_widget)
        m_widget = new DeviceManagerWidget(*this, Core::ICore::instance()->dialogParent());

    return m_widget.get();
}

} // namespace QmlDesigner::DeviceShare
