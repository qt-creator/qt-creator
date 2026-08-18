// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <memory>

namespace HarmonyOs::Internal {

// A HarmonyOS target device. Like Android, it derives from IDevice rather than
// DesktopDevice: it must not present itself as a local file handler, or it
// would hijack local-path resolution (e.g. break CMake tool detection).
class HarmonyOsDevice final : public ProjectExplorer::IDevice
{
public:
    ~HarmonyOsDevice() final;

    static ProjectExplorer::IDevice::Ptr create();

    ProjectExplorer::IDeviceWidget *createWidget() final;
    bool hasDeviceTester() const final { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() final;
    Utils::ProcessInterface *createProcessInterface() const final;

    QString serialNumber() const;
    void setSerialNumber(const QString &serial);

    void updateFileAccess();

private:
    HarmonyOsDevice();

    class Private;
    const std::unique_ptr<Private> d;
};

class HarmonyOsDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    HarmonyOsDeviceFactory();
};

void setupHarmonyOsDevice();
void setupHarmonyOsDeviceDetection();

} // namespace HarmonyOs::Internal
