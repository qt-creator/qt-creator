// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDebug>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <projectexplorer/devicesupport/idevice.h>

namespace Android::Internal {

class AndroidDeviceInfo
{
public:
    bool isValid() const { return !serialNumber.isEmpty() || !avdName.isEmpty(); }

    QString serialNumber;
    QString avdName;
    QStringList cpuAbi;
    int sdk = -1;
    ProjectExplorer::IDevice::DeviceState state = ProjectExplorer::IDevice::DeviceDisconnected;
    ProjectExplorer::IDevice::MachineType type = ProjectExplorer::IDevice::Emulator;
    Utils::FilePath avdPath;

private:
    friend bool operator<(const AndroidDeviceInfo &lhs, const AndroidDeviceInfo &rhs);
    friend bool operator==(const AndroidDeviceInfo &lhs, const AndroidDeviceInfo &rhs) = default;
    friend bool operator!=(const AndroidDeviceInfo &lhs, const AndroidDeviceInfo &rhs) = default;
    friend QDebug &operator<<(QDebug &stream, const AndroidDeviceInfo &device);
};

using AndroidDeviceInfoList = QList<AndroidDeviceInfo>;

} // namespace Android::Internal

Q_DECLARE_METATYPE(Android::Internal::AndroidDeviceInfo)
