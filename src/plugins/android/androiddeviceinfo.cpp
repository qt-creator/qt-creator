// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androiddeviceinfo.h"

using namespace ProjectExplorer;

namespace Android::Internal {

bool operator<(const AndroidDeviceInfo &lhs, const AndroidDeviceInfo &rhs)
{
    if (lhs.serialNumber.contains("????") != rhs.serialNumber.contains("????"))
        return !lhs.serialNumber.contains("????");
    if (lhs.type != rhs.type)
        return lhs.type == IDevice::Hardware;
    if (lhs.sdk != rhs.sdk)
        return lhs.sdk < rhs.sdk;
    if (lhs.avdName != rhs.avdName)
        return lhs.avdName < rhs.avdName;

    return lhs.serialNumber < rhs.serialNumber;
}

QDebug &operator<<(QDebug &stream, const AndroidDeviceInfo &device)
{
    stream.nospace()
           << "Type:" << (device.type == IDevice::Emulator ? "Emulator" : "Device")
           << ", ABI:" << device.cpuAbi << ", Serial:" << device.serialNumber
           << ", Name:" << device.avdName << ", API:" << device.sdk
           << ", Authorised:" << (device.state == IDevice::DeviceReadyToUse);
    return stream;
}

} // namespace Android::Internal
