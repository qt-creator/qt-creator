// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyconstants.h"
#include "webassemblydevice.h"
#include "webassemblytr.h"

#include <projectexplorer/devicesupport/desktopdevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

class WebAssemblyDevice final : public DesktopDevice
{
public:
    WebAssemblyDevice()
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
};

IDevicePtr createWebAssemblyDevice()
{
    return IDevicePtr(new WebAssemblyDevice);
}

class WebAssemblyDeviceFactory final : public IDeviceFactory
{
public:
    WebAssemblyDeviceFactory()
        : IDeviceFactory(Constants::WEBASSEMBLY_DEVICE_TYPE)
    {
        setDisplayName(Tr::tr("WebAssembly Runtime"));
        setCombinedIcon(":/webassembly/images/webassemblydevicesmall.png",
                        ":/webassembly/images/webassemblydevice.png");
        setConstructionFunction(&createWebAssemblyDevice);
        setCreator(&createWebAssemblyDevice);
    }
};

void setupWebAssemblyDevice()
{
    static WebAssemblyDeviceFactory theWebAssemblyDeviceFactory;
}

} // WebAssembly::Internal
