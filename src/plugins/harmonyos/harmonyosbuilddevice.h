// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <remote/linuxdevice.h>

#include <projectexplorer/devicesupport/idevicefactory.h>

namespace HarmonyOs::Internal {

// A HarmonyOS machine to build on, reached over SSH. The device runs a native
// toolchain in a user environment that hdc cannot see, so this is a separate device
// from the one applications are deployed to, and reuses LinuxDevice's SSH machinery.
class HarmonyOsBuildDevice : public Remote::LinuxDevice
{
public:
    using Ptr = std::shared_ptr<HarmonyOsBuildDevice>;

    static Ptr create() { return Ptr(new HarmonyOsBuildDevice); }

protected:
    HarmonyOsBuildDevice();
};

class HarmonyOsBuildDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    HarmonyOsBuildDeviceFactory();
};

void setupHarmonyOsBuildDevice();

} // namespace HarmonyOs::Internal
