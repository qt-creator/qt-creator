// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Remote {

// A remote Windows machine reached over SSH. Unlike LinuxDevice, all file and
// process operations are expressed in native Windows terms (PowerShell, backslash
// paths) instead of a POSIX shell. The SSH transport itself is shared with
// LinuxDevice via the project-wide SshParameters / sshSettings() machinery.
class REMOTELINUX_EXPORT WindowsDevice : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<WindowsDevice>;
    using ConstPtr = std::shared_ptr<const WindowsDevice>;

    ~WindowsDevice() override;

    static Ptr create() { return Ptr(new WindowsDevice); }

    ProjectExplorer::IDeviceWidget *createWidget() override;

    bool hasDeviceTester() const override { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() override;

    QString userAtHost() const;
    QString userAtHostAndPort() const;

    Utils::FilePath rootPath() const override;

    Utils::Result<> handlesFile(const Utils::FilePath &filePath) const override;

    Utils::ProcessInterface *createProcessInterface() const override;

    void tryToConnect(const Utils::Continuation<> &cont) const override;

protected:
    WindowsDevice();

    class WindowsDevicePrivate *d;
    friend class WindowsDevicePrivate;
};

namespace Internal {

class WindowsDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    WindowsDeviceFactory();
};

#ifdef WITH_TESTS
// Test-only entry point that runs the same MSVC toolchain auto-detection and kit
// creation the device widget's "Run Auto-Detection Now" button triggers.
void detectAndRegisterToolchainsForTest(const ProjectExplorer::IDevice::Ptr &device);
#endif

} // namespace Internal
} // namespace Remote
