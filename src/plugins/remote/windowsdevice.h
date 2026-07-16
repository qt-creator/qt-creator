// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/aspects.h>

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
    void runAutoDetect(
        const ProjectExplorer::ToolDetectionLogger &logger,
        const std::function<void()> &onDone) override;

    bool hasDeviceTester() const override { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() override;

    QString userAtHost() const;
    QString userAtHostAndPort() const;

    Utils::Result<Utils::OsArch> osArch() const;

    Utils::FilePath rootPath() const override;

    Utils::Result<> handlesFile(const Utils::FilePath &filePath) const override;

    Utils::ProcessInterface *createProcessInterface() const override;

    void tryToConnect(const Utils::Continuation<> &cont) const override;

    void postLoad() override;

    // Connect the device on startup (see postLoad), so it has file access and shows up in
    // device-aware file dialogs. On by default; auto-disabled after a failed auto-connect.
    Utils::BoolAspect autoConnectOnStartup{this};

    // Device directory that holds the CDB helper extension's per-arch subdirectories
    // (qtcreatorcdbext{,arm}{32,64}), i.e. the "lib" directory of a Qt Creator installed on the
    // device. Used for debugging the device's binaries with a remote cdb.exe; empty disables it.
    Utils::FilePathAspect cdbExtensionDirectory{this};

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

} // namespace Internal
} // namespace Remote
