// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androiddeviceinfo.h"

namespace Android {

/**
 * Workaround for '????????????' serial numbers
 * @return ("-d") for buggy devices, ("-s", <serial no>) for normal
 */
QStringList AndroidDeviceInfo::adbSelector(const QString &serialNumber)
{
    if (serialNumber.startsWith(QLatin1String("????")))
        return {"-d"};
    return {"-s", serialNumber};
}

bool AndroidDeviceInfo::operator<(const AndroidDeviceInfo &other) const
{
    if (serialNumber.contains("????") != other.serialNumber.contains("????"))
        return !serialNumber.contains("????");
    if (type != other.type)
        return type == ProjectExplorer::IDevice::Hardware;
    if (sdk != other.sdk)
        return sdk < other.sdk;
    if (avdName != other.avdName)
        return avdName < other.avdName;

    return serialNumber < other.serialNumber;
}

bool AndroidDeviceInfo::operator==(const AndroidDeviceInfo &other) const
{
    return serialNumber == other.serialNumber && avdName == other.avdName
            && avdPath == other.avdPath && cpuAbi == other.cpuAbi
            && sdk == other.sdk && state == other.state && type == other.type;
}

QDebug &operator<<(QDebug &stream, const AndroidDeviceInfo &device)
{
    stream.nospace()
           << "Type:" << (device.type == ProjectExplorer::IDevice::Emulator ? "Emulator" : "Device")
           << ", ABI:" << device.cpuAbi << ", Serial:" << device.serialNumber
           << ", Name:" << device.avdName << ", API:" << device.sdk
           << ", Authorised:" << (device.state == IDevice::DeviceReadyToUse);
    return stream;
}

} // namespace Android
