/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "webassemblyconstants.h"
#include "webassemblydevice.h"

#include <projectexplorer/devicesupport/deviceprocess.h>
#include <projectexplorer/runcontrol.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

WebAssemblyDevice::WebAssemblyDevice()
{
    setupId(IDevice::AutoDetected, Constants::WEBASSEMBLY_DEVICE_DEVICE_ID);
    setType(Constants::WEBASSEMBLY_DEVICE_TYPE);
    const QString displayNameAndType = tr("Web Browser");
    setDefaultDisplayName(displayNameAndType);
    setDisplayType(displayNameAndType);
    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(OsTypeOther);
}

ProjectExplorer::IDevice::Ptr WebAssemblyDevice::create()
{
    auto device = new WebAssemblyDevice;
    return ProjectExplorer::IDevice::Ptr(device);
}

WebAssemblyDeviceFactory::WebAssemblyDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::WEBASSEMBLY_DEVICE_TYPE)
{
    setDisplayName(WebAssemblyDevice::tr("WebAssembly Runtime"));
    setCombinedIcon(":/webassembly/images/webassemblydevicesmall.png",
                    ":/webassembly/images/webassemblydevice.png");
    setCanCreate(true);
    setConstructionFunction(&WebAssemblyDevice::create);
}

ProjectExplorer::IDevice::Ptr WebAssemblyDeviceFactory::create() const
{
    return WebAssemblyDevice::create();
}

} // namespace Internal
} // namespace WebAssembly
