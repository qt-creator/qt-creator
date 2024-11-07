// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanagermodel.h"
#include "devicemanager.h"

namespace QmlDesigner::DeviceShare {

DeviceManagerModel::DeviceManagerModel(DeviceManager &deviceManager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_deviceManager(deviceManager)
{
    connect(&m_deviceManager, &DeviceManager::deviceAdded, this, [this](const DeviceInfo &) {
        beginResetModel();
        endResetModel();
    });
    connect(&m_deviceManager, &DeviceManager::deviceRemoved, this, [this](const DeviceInfo &) {
        beginResetModel();
        endResetModel();
    });

    connect(&m_deviceManager, &DeviceManager::deviceOnline, this, [this](const DeviceInfo &) {
        beginResetModel();
        endResetModel();
    });

    connect(&m_deviceManager, &DeviceManager::deviceOffline, this, [this](const DeviceInfo &) {
        beginResetModel();
        endResetModel();
    });
}

int DeviceManagerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_deviceManager.devices().size();
}

int DeviceManagerModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return DeviceColumns::COLUMN_COUNT;
}

QVariant DeviceManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        const auto deviceInfo = m_deviceManager.devices()[index.row()]->deviceInfo();
        const auto deviceSettings = m_deviceManager.devices()[index.row()]->deviceSettings();
        bool isConnected = m_deviceManager.devices()[index.row()]->isConnected();
        switch (index.column()) {
        case DeviceColumns::Active:
            return deviceSettings.active();
        case DeviceColumns::Status:
            return static_cast<DeviceStatus>(isConnected);
        case DeviceColumns::Alias:
            return deviceSettings.alias();
        case DeviceColumns::IPv4Addr:
            return deviceSettings.ipAddress();
        case DeviceColumns::OS:
            return deviceInfo.os();
        case DeviceColumns::OSVersion:
            return deviceInfo.osVersion();
        case DeviceColumns::Architecture:
            return deviceInfo.architecture();
        case DeviceColumns::ScreenSize:
            return QString("%1x%2").arg(deviceInfo.screenWidth()).arg(deviceInfo.screenHeight());
        case DeviceColumns::AppVersion:
            return deviceInfo.appVersion();
        case DeviceColumns::DeviceId:
            return deviceInfo.deviceId();
        }
    }

    if (role == Qt::EditRole) {
        const auto deviceSettings = m_deviceManager.devices()[index.row()]->deviceSettings();
        switch (index.column()) {
        case DeviceColumns::Alias:
            return deviceSettings.alias();
        case DeviceColumns::Active:
            return deviceSettings.active();
        }
    }

    return QVariant();
}

QVariant DeviceManagerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case DeviceColumns::Active:
            return tr("Active");
        case DeviceColumns::Status:
            return tr("Status");
        case DeviceColumns::Alias:
            return tr("Alias");
        case DeviceColumns::IPv4Addr:
            return tr("IPv4 Address");
        case DeviceColumns::OS:
            return tr("OS");
        case DeviceColumns::OSVersion:
            return tr("OS Version");
        case DeviceColumns::Architecture:
            return tr("Architecture");
        case DeviceColumns::ScreenSize:
            return tr("Screen Size");
        case DeviceColumns::AppVersion:
            return tr("App Version");
        case DeviceColumns::DeviceId:
            return tr("Device ID");
        }
    }

    return QVariant();
}

bool DeviceManagerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole) {
        qCWarning(deviceSharePluginLog) << "Invalid index or role";
        return false;
    }

    auto deviceInfo = m_deviceManager.devices()[index.row()]->deviceInfo();
    switch (index.column()) {
    case DeviceColumns::Alias:
        m_deviceManager.setDeviceAlias(deviceInfo.deviceId(), value.toString());
        break;
    case DeviceColumns::Active:
        m_deviceManager.setDeviceActive(deviceInfo.deviceId(), value.toBool());
        break;
    case DeviceColumns::IPv4Addr:
        m_deviceManager.setDeviceIP(deviceInfo.deviceId(), value.toString());
        break;
    }
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags DeviceManagerModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == DeviceColumns::Active || index.column() == DeviceColumns::Alias)
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool DeviceManagerModel::addDevice(const QString &ip)
{
    return m_deviceManager.addDevice(ip);
}

void DeviceManagerModel::removeDevice(const QString &deviceId)
{
    m_deviceManager.removeDevice(deviceId);
}

} // namespace QmlDesigner::DeviceShare
