// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanagermodel.h"
#include "device.h"
#include "devicemanager.h"

#include <qmldesignertr.h>

namespace QmlDesigner::DeviceShare {

DeviceManagerModel::DeviceManagerModel(DeviceManager &deviceManager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_deviceManager(deviceManager)
{
    connect(&m_deviceManager, &DeviceManager::deviceAdded, this, [this](const QString &) {
        beginResetModel();
        endResetModel();
    });
    connect(&m_deviceManager, &DeviceManager::deviceRemoved, this, [this](const QString &) {
        beginResetModel();
        endResetModel();
    });

    connect(&m_deviceManager, &DeviceManager::deviceOnline, this, [this](const QString &) {
        beginResetModel();
        endResetModel();
    });

    connect(&m_deviceManager, &DeviceManager::deviceOffline, this, [this](const QString &) {
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
        case DeviceColumns::Enabled:
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
        case DeviceColumns::SelfId:
            return deviceInfo.selfId();
        case DeviceColumns::DeviceId:
            return deviceSettings.deviceId();
        }
    }

    if (role == Qt::EditRole) {
        const auto deviceSettings = m_deviceManager.devices()[index.row()]->deviceSettings();
        switch (index.column()) {
        case DeviceColumns::Alias:
            return deviceSettings.alias();
        case DeviceColumns::Enabled:
            return deviceSettings.active();
        case DeviceColumns::IPv4Addr:
            return deviceSettings.ipAddress();
        }
    }

    return QVariant();
}

QVariant DeviceManagerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::ToolTipRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case DeviceColumns::Enabled:
                return Tr::tr("Enables or disables the targeted device in the Run dropdown.");
            case DeviceColumns::Status:
                return Tr::tr("Indicates whether the Qt UI Viewer on the targeted device is turned "
                              "on or off.");
            case DeviceColumns::Alias:
                return Tr::tr("Sets the name of the targeted device.");
            case DeviceColumns::IPv4Addr:
                return Tr::tr("Displays the IP address of the targeted device.");
            case DeviceColumns::OS:
                return Tr::tr("Displays the operating system of the targeted device.");
            case DeviceColumns::OSVersion:
                return Tr::tr(
                    "Displays the version of the operating system on the targeted device.");
            case DeviceColumns::Architecture:
                return Tr::tr("Displays the CPU architecture information of the targeted device.");
            case DeviceColumns::ScreenSize:
                return Tr::tr("Displays the screen dimensions of the targeted device.");
            case DeviceColumns::AppVersion:
                return Tr::tr("Displays the version ID of the Qt UI Viewer application.");
            case DeviceColumns::SelfId:
                return Tr::tr("Displays the ID created by the target device.");
            case DeviceColumns::DeviceId:
                return Tr::tr("Displays the ID created by Qt Design Studio for the target device.");
            }
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case DeviceColumns::Enabled:
            return Tr::tr("Enabled");
        case DeviceColumns::Status:
            return Tr::tr("Status");
        case DeviceColumns::Alias:
            return Tr::tr("Alias");
        case DeviceColumns::IPv4Addr:
            return Tr::tr("IP Address");
        case DeviceColumns::OS:
            return Tr::tr("OS");
        case DeviceColumns::OSVersion:
            return Tr::tr("OS Version");
        case DeviceColumns::Architecture:
            return Tr::tr("Architecture");
        case DeviceColumns::ScreenSize:
            return Tr::tr("Screen Size");
        case DeviceColumns::AppVersion:
            return Tr::tr("App Version");
        case DeviceColumns::SelfId:
            return Tr::tr("Self ID");
        case DeviceColumns::DeviceId:
            return Tr::tr("Device ID");
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

    auto deviceSettings = m_deviceManager.devices()[index.row()]->deviceSettings();
    switch (index.column()) {
    case DeviceColumns::Alias:
        m_deviceManager.setDeviceAlias(deviceSettings.deviceId(), value.toString());
        break;
    case DeviceColumns::Enabled:
        m_deviceManager.setDeviceActive(deviceSettings.deviceId(), value.toBool());
        break;
    case DeviceColumns::IPv4Addr:
        m_deviceManager.setDeviceIP(deviceSettings.deviceId(), value.toString());
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

    if (index.column() == DeviceColumns::Enabled || index.column() == DeviceColumns::Alias
        || index.column() == DeviceColumns::IPv4Addr)
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
