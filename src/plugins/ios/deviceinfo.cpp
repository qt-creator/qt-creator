// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceinfo.h"

#include "devicectlutils.h"
#include "iostr.h"

#include <QHash>

inline constexpr char kDeveloperStatus[] = "developerStatus";
inline constexpr char kIsPaired[] = "isPaired";
inline constexpr char kDeviceConnected[] = "deviceConnected";
inline constexpr char kOsVersion[] = "osVersion";
inline constexpr char kProductType[] = "productType";
inline constexpr char kCpuArchitecture[] = "cpuArchitecture";
inline constexpr char kUniqueDeviceId[] = "uniqueDeviceId";
inline constexpr char vOff[] = "*off*";
inline constexpr char vUnknown[] = "*unknown*";
inline constexpr char vDevelopment[] = "Development";
inline constexpr char vYes[] = "YES";
inline constexpr char vNo[] = "NO";

namespace Ios::Internal {

IosDeviceInfo IosDeviceInfo::fromMap(const QMap<QString, QString> &map)
{
    IosDeviceInfo info;
    info.deviceName = map.value(kDeviceName);
    const QString developerStatus = map.value(kDeveloperStatus);
    if (developerStatus == QLatin1String(vDevelopment))
        info.developmentStatus = DevelopmentStatus::Enabled;
    else if (developerStatus == QLatin1String(vOff))
        info.developmentStatus = DevelopmentStatus::Disabled;
    else
        info.developmentStatus = DevelopmentStatus::Unknown;
    if (map.contains(kIsPaired))
        info.isPaired = map.value(kIsPaired) == vYes;
    info.deviceConnected = map.value(kDeviceConnected) == QLatin1String(vYes);
    info.osVersion = map.value(kOsVersion);
    info.productType = map.value(kProductType);
    info.cpuArchitecture = map.value(kCpuArchitecture);
    info.uniqueDeviceId = map.value(kUniqueDeviceId);
    return info;
}

QMap<QString, QString> IosDeviceInfo::toMap() const
{
    QMap<QString, QString> map;
    map[kDeviceName] = deviceName;
    switch (developmentStatus) {
    case DevelopmentStatus::Enabled:
        map[kDeveloperStatus] = QLatin1String(vDevelopment);
        break;
    case DevelopmentStatus::Disabled:
        map[kDeveloperStatus] = QLatin1String(vOff);
        break;
    case DevelopmentStatus::Unknown:
        map[kDeveloperStatus] = QLatin1String(vUnknown);
        break;
    }
    if (isPaired.has_value())
        map[kIsPaired] = QLatin1String(*isPaired ? vYes : vNo);
    map[kDeviceConnected] = QLatin1String(deviceConnected ? vYes : vNo);
    map[kOsVersion] = osVersion;
    map[kProductType] = productType;
    map[kCpuArchitecture] = cpuArchitecture;
    map[kUniqueDeviceId] = uniqueDeviceId;
    return map;
}

ProjectExplorer::IDevice::DeviceInfo IosDeviceInfo::toDeviceInfo() const
{
    using ProjectExplorer::IDevice;

    static const QHash<QString, QString> valueTranslations = {
        {"*unknown*", Tr::tr("unknown")},
    };
    const auto translate = [](const QString &value) {
        return valueTranslations.value(value, value);
    };

    QString developmentStatusText;
    switch (developmentStatus) {
    case DevelopmentStatus::Enabled:
        //: Whether the device is in developer mode.
        developmentStatusText = Tr::tr("enabled");
        break;
    case DevelopmentStatus::Disabled:
        //: Whether the device is in developer mode.
        developmentStatusText = Tr::tr("disabled");
        break;
    case DevelopmentStatus::Unknown:
        //: Whether the device is in developer mode.
        developmentStatusText = Tr::tr("unknown");
        break;
    }
    const auto boolString = [](bool v) { return v ? Tr::tr("yes") : Tr::tr("no"); };

    IDevice::DeviceInfo result;
    result.append({Tr::tr("Device name"), translate(deviceName)});
    result.append({Tr::tr("Identifier"), uniqueDeviceId});
    result.append({Tr::tr("Product type"), translate(productType)});
    result.append({Tr::tr("OS version"), translate(osVersion)});
    result.append({Tr::tr("CPU architecture"), cpuArchitecture});
    if (isPaired)
        result.append({Tr::tr("Paired"), boolString(*isPaired)});
    result.append({Tr::tr("Developer status"), developmentStatusText});
    return result;
}

QDebug operator<<(QDebug debug, const IosDeviceInfo &info)
{
    const char *developmentStatus = "Unknown";
    switch (info.developmentStatus) {
    case IosDeviceInfo::DevelopmentStatus::Enabled:
        developmentStatus = "Enabled";
        break;
    case IosDeviceInfo::DevelopmentStatus::Disabled:
        developmentStatus = "Disabled";
        break;
    case IosDeviceInfo::DevelopmentStatus::Unknown:
        developmentStatus = "Unknown";
        break;
    }
    debug << "IosDeviceInfo(deviceName=" << info.deviceName
          << ", developmentStatus=" << developmentStatus
          << ", deviceConnected=" << info.deviceConnected << ", osVersion=" << info.osVersion
          << ", productType=" << info.productType
          << ", cpuArchitecture=" << info.cpuArchitecture
          << ", uniqueDeviceId=" << info.uniqueDeviceId << ")";
    return debug;
}

} // namespace Ios::Internal
