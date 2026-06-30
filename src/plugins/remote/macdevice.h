// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "linuxdevice.h"

#include <utils/synchronizedvalue.h>

namespace Remote {

// A remote macOS machine reached over SSH. macOS is a POSIX system, so a Mac device
// reuses all of LinuxDevice's Unix machinery (SSH transport, shared connections,
// cmdbridge / UnixDeviceFileAccess, process interface and device tester). Only the
// device type, OS type, the user-visible strings and the process-kill command (macOS
// has no /proc) differ from LinuxDevice.
class REMOTELINUX_EXPORT MacDevice : public LinuxDevice
{
public:
    using Ptr = std::shared_ptr<MacDevice>;
    using ConstPtr = std::shared_ptr<const MacDevice>;

    static Ptr create() { return Ptr(new MacDevice); }

protected:
    MacDevice();
};

namespace Internal {

class MacDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    MacDeviceFactory();
    ~MacDeviceFactory() override;

private:
    Utils::SynchronizedValue<std::vector<std::weak_ptr<MacDevice>>> m_existingDevices;
    void shutdownExistingDevices();
};

} // namespace Internal
} // namespace Remote
