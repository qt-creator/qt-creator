// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/desktopdevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace HarmonyOs::Internal {

class HarmonyOsDevice final : public ProjectExplorer::DesktopDevice
{
public:
    static ProjectExplorer::IDevice::Ptr create();

    Utils::Result<> handlesFile(const Utils::FilePath &filePath) const final;

private:
    HarmonyOsDevice();
};

class HarmonyOsDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    HarmonyOsDeviceFactory();
};

void setupHarmonyOsDevice();

} // namespace HarmonyOs::Internal
