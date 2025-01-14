// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QLoggingCategory>

namespace QmlDesigner::DeviceShare {
Q_DECLARE_LOGGING_CATEGORY(deviceSharePluginLog);

class IDeviceData
{
public:
    IDeviceData(const QJsonObject &data = QJsonObject());
    virtual ~IDeviceData() = default;

    // converters
    QJsonObject jsonObject() const;
    void setJsonObject(const QJsonObject &data);
    operator QString() const;

protected:
    QJsonObject m_data;
};

class DeviceSettings : public IDeviceData
{
public:
    using IDeviceData::IDeviceData;

    // Getters
    bool active() const;
    QString alias() const;
    QString ipAddress() const;
    QString deviceId() const;

    // Setters
    void setActive(const bool &active);
    void setAlias(const QString &alias);
    void setIpAddress(const QString &ipAddress);
    void setDeviceId(const QString &deviceId);

private:
    static constexpr char keyActive[] = "deviceActive";
    static constexpr char keyAlias[] = "deviceAlias";
    static constexpr char keyIpAddress[] = "ipAddress";
    static constexpr char keyDeviceId[] = "deviceId";
};

class DeviceInfo : public IDeviceData
{
public:
    using IDeviceData::IDeviceData;

    // Getters
    QString os() const;
    QString osVersion() const;
    QString architecture() const;
    int screenWidth() const;
    int screenHeight() const;
    QString selfId() const;
    QString appVersion() const;

    // Setters
    void setOs(const QString &os);
    void setOsVersion(const QString &osVersion);
    void setArchitecture(const QString &architecture);
    void setScreenWidth(const int &screenWidth);
    void setScreenHeight(const int &screenHeight);
    void setSelfId(const QString &selfId);
    void setAppVersion(const QString &appVersion);

private:
    static constexpr char keyOs[] = "os";
    static constexpr char keyOsVersion[] = "osVersion";
    static constexpr char keyScreenWidth[] = "screenWidth";
    static constexpr char keyScreenHeight[] = "screenHeight";
    static constexpr char keySelfId[] = "deviceId";
    static constexpr char keyArchitecture[] = "architecture";
    static constexpr char keyAppVersion[] = "appVersion";
};

} // namespace QmlDesigner::DeviceShare
