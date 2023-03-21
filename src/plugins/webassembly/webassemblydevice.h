// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/desktopdevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace WebAssembly {
namespace Internal {

class WebAssemblyDevice final : public ProjectExplorer::DesktopDevice
{
public:
    static ProjectExplorer::IDevice::Ptr create();

private:
    WebAssemblyDevice();
};

class WebAssemblyDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    WebAssemblyDeviceFactory();
};

} // namespace Internal
} // namespace WebAssembly
