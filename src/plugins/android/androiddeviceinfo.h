/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
