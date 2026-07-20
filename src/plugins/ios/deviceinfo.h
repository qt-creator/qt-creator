// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <QDebug>
#include <QMap>
#include <QString>

namespace Ios::Internal {

struct IosDeviceInfo
{
    enum class DevelopmentStatus { Enabled, Disabled, Unknown };

    static IosDeviceInfo fromMap(const QMap<QString, QString> &map);
    QMap<QString, QString> toMap() const;
    ProjectExplorer::IDevice::DeviceInfo toDeviceInfo() const;

    bool operator==(const IosDeviceInfo &other) const = default;

    QString deviceName;
    DevelopmentStatus developmentStatus = DevelopmentStatus::Unknown;
    bool deviceConnected = false;
    QString osVersion;
    QString productType;
    QString cpuArchitecture;
    QString uniqueDeviceId;
};

QDebug operator<<(QDebug debug, const IosDeviceInfo &info);

} // namespace Ios::Internal

Q_DECLARE_METATYPE(Ios::Internal::IosDeviceInfo)
