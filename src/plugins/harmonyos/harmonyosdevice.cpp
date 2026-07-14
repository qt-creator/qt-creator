// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdevice.h"

#include "harmonyosconstants.h"
#include "harmonyostr.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace HarmonyOs::Internal {

HarmonyOsDevice::HarmonyOsDevice()
{
    setType(Constants::HARMONYOS_DEVICE_TYPE);
    const QString displayNameAndType = Tr::tr("HarmonyOS Device");
    setDefaultDisplayName(displayNameAndType);
    setDisplayType(displayNameAndType);
    setMachineType(IDevice::Hardware);
    setOsType(OsTypeOther);
    setDeviceState(IDevice::DeviceStateUnknown);
}

IDevice::Ptr HarmonyOsDevice::create()
{
    return IDevice::Ptr(new HarmonyOsDevice);
}

Result<> HarmonyOsDevice::handlesFile(const FilePath &) const
{
    return ResultError(Tr::tr("File handling is not supported."));
}

HarmonyOsDeviceFactory::HarmonyOsDeviceFactory()
    : IDeviceFactory(Constants::HARMONYOS_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("HarmonyOS Device"));
    setConstructionFunction(&HarmonyOsDevice::create);
}

void setupHarmonyOsDevice()
{
    static HarmonyOsDeviceFactory theHarmonyOsDeviceFactory;
}

} // namespace HarmonyOs::Internal
