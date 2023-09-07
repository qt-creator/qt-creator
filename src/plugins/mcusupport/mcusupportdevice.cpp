// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportdevice.h"
#include "mcusupportconstants.h"
#include "mcusupporttr.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

McuSupportDevice::McuSupportDevice()
{
    setupId(IDevice::AutoDetected, Constants::DEVICE_ID);
    setType(Constants::DEVICE_TYPE);
    const QString displayNameAndType = Tr::tr("MCU Device");
    settings()->displayName.setDefaultValue(displayNameAndType);
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
    setDisplayName(Tr::tr("MCU Device"));
    setCombinedIcon(":/mcusupport/images/mcusupportdevicesmall.png",
                    ":/mcusupport/images/mcusupportdevice.png");
    setConstructionFunction(&McuSupportDevice::create);
    setCreator(&McuSupportDevice::create);
}

} // namespace Internal
} // namespace McuSupport
