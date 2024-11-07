// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceinfo.h"

#include <QJsonDocument>

namespace QmlDesigner::DeviceShare {
Q_LOGGING_CATEGORY(deviceSharePluginLog, "qtc.designer.deviceSharePluginLog")

IDeviceData::IDeviceData(const QJsonObject &data)
    : m_data(data)
{}

QJsonObject IDeviceData::jsonObject() const
{
    return m_data;
}

void IDeviceData::setJsonObject(const QJsonObject &data)
{
    m_data = data;
}

IDeviceData::operator QString() const
{
    return QString::fromLatin1(QJsonDocument(m_data).toJson(QJsonDocument::Compact));
}

bool DeviceSettings::active() const
{
    return m_data.value(keyActive).toBool(true);
}

QString DeviceSettings::alias() const
{
    return m_data.value(keyAlias).toString();
}

QString DeviceSettings::ipAddress() const
{
    return m_data.value(keyIpAddress).toString();
}

void DeviceSettings::setActive(const bool &active)
{
    m_data[keyActive] = active;
}

void DeviceSettings::setAlias(const QString &alias)
{
    m_data[keyAlias] = alias;
}

void DeviceSettings::setIpAddress(const QString &ipAddress)
{
    m_data[keyIpAddress] = ipAddress;
}

QString DeviceInfo::os() const
{
    return m_data.value(keyOs).toString();
}

QString DeviceInfo::osVersion() const
{
    return m_data.value(keyOsVersion).toString();
}

QString DeviceInfo::architecture() const
{
    return m_data.value(keyArchitecture).toString();
}

int DeviceInfo::screenWidth() const
{
    return m_data.value(keyScreenWidth).toInt();
}

int DeviceInfo::screenHeight() const
{
    return m_data.value(keyScreenHeight).toInt();
}

QString DeviceInfo::deviceId() const
{
    return m_data.value(keyDeviceId).toString();
}

QString DeviceInfo::appVersion() const
{
    return m_data.value(keyAppVersion).toString();
}

void DeviceInfo::setOs(const QString &os)
{
    m_data[keyOs] = os;
}

void DeviceInfo::setOsVersion(const QString &osVersion)
{
    m_data[keyOsVersion] = osVersion;
}

void DeviceInfo::setArchitecture(const QString &architecture)
{
    m_data[keyArchitecture] = architecture;
}

void DeviceInfo::setScreenWidth(const int &screenWidth)
{
    m_data[keyScreenWidth] = screenWidth;
}

void DeviceInfo::setScreenHeight(const int &screenHeight)
{
    m_data[keyScreenHeight] = screenHeight;
}

void DeviceInfo::setDeviceId(const QString &deviceId)
{
    m_data[keyDeviceId] = deviceId;
}

void DeviceInfo::setAppVersion(const QString &appVersion)
{
    m_data[keyAppVersion] = appVersion;
}

} // namespace QmlDesigner::DeviceShare
