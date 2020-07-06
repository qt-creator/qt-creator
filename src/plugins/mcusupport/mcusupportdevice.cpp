/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupportconstants.h"
#include "mcusupportdevice.h"

#include <projectexplorer/devicesupport/deviceprocess.h>
#include <projectexplorer/runcontrol.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

McuSupportDevice::McuSupportDevice()
{
    setupId(IDevice::AutoDetected, Constants::DEVICE_ID);
    setType(Constants::DEVICE_TYPE);
    const QString displayNameAndType = tr("MCU Device");
    setDefaultDisplayName(displayNameAndType);
    setDisplayType(displayNameAndType);
    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(Utils::OsTypeOther);
}

ProjectExplorer::IDevice::Ptr McuSupportDevice::create()
{
    auto device = new McuSupportDevice;
    return ProjectExplorer::IDevice::Ptr(device);
}

McuSupportDeviceFactory::McuSupportDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::DEVICE_TYPE)
{
    setDisplayName(McuSupportDevice::tr("MCU Device"));
    setCombinedIcon(":/mcusupport/images/mcusupportdevicesmall.png",
                    ":/mcusupport/images/mcusupportdevice.png");
    setCanCreate(true);
    setConstructionFunction(&McuSupportDevice::create);
}

ProjectExplorer::IDevice::Ptr McuSupportDeviceFactory::create() const
{
    return McuSupportDevice::create();
}

} // namespace Internal
} // namespace McuSupport
