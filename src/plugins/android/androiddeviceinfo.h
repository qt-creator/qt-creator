// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDebug>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <projectexplorer/devicesupport/idevice.h>

using namespace ProjectExplorer;

namespace Android {

class AndroidDeviceInfo
{
public:
    QString serialNumber;
    QString avdName;
    QStringList cpuAbi;
    int sdk = -1;
    IDevice::DeviceState state = IDevice::DeviceDisconnected;
    IDevice::MachineType type = IDevice::Emulator;
    Utils::FilePath avdPath;

    static QStringList adbSelector(const QString &serialNumber);

    bool isValid() const { return !serialNumber.isEmpty() || !avdName.isEmpty(); }
    bool operator<(const AndroidDeviceInfo &other) const;
    bool operator==(const AndroidDeviceInfo &other) const; // should be = default with C++20
    bool operator!=(const AndroidDeviceInfo &other) const { return !(*this == other); }
};
using AndroidDeviceInfoList = QList<AndroidDeviceInfo>;

QDebug &operator<<(QDebug &stream, const AndroidDeviceInfo &device);

} // namespace Android

Q_DECLARE_METATYPE(Android::AndroidDeviceInfo)
