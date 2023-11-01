// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyconstants.h"
#include "webassemblydevice.h"
#include "webassemblytr.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

WebAssemblyDevice::WebAssemblyDevice()
{
    setupId(IDevice::AutoDetected, Constants::WEBASSEMBLY_DEVICE_DEVICE_ID);
    setType(Constants::WEBASSEMBLY_DEVICE_TYPE);
    const QString displayNameAndType = Tr::tr("Web Browser");
    settings()->displayName.setDefaultValue(displayNameAndType);
    setDisplayType(displayNameAndType);
    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(OsTypeOther);
    setFileAccess(nullptr);
}

IDevice::Ptr WebAssemblyDevice::create()
{
    return IDevicePtr(new WebAssemblyDevice);
}

WebAssemblyDeviceFactory::WebAssemblyDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::WEBASSEMBLY_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("WebAssembly Runtime"));
    setCombinedIcon(":/webassembly/images/webassemblydevicesmall.png",
                    ":/webassembly/images/webassemblydevice.png");
    setConstructionFunction(&WebAssemblyDevice::create);
    setCreator(&WebAssemblyDevice::create);
}

} // namespace Internal
} // namespace WebAssembly
