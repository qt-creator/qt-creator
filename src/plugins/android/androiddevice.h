/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Android {
namespace Internal {

class AndroidDevice final : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidDevice)

public:
    static IDevice::Ptr create() { return IDevice::Ptr(new AndroidDevice); }

private:
    AndroidDevice();

    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    bool canAutoDetectPorts() const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    QUrl toolControlChannel(const ControlChannelHint &) const override;
};

class AndroidDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    AndroidDeviceFactory();
};

} // namespace Internal
} // namespace Android
