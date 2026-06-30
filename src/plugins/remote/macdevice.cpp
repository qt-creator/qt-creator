// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macdevice.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "sshdevicewizard.h"

#include <projectexplorer/projectexplorersettings.h>

#include <utils/filepath.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote {

// Like killCommandForPath() in linuxdevice.cpp, but uses "ps" to locate the process by
// its executable path: macOS has no /proc, so the readlink(/proc/$p/exe) approach used
// for Linux does not apply. On macOS "ps -axo comm=" prints the full executable path.
static QString macKillCommandForPath(const FilePath &filePath)
{
    return QString::fromLatin1(R"(
        pid=`ps -axo pid=,comm= | while read p c
        do
          if [ "$c" = "%1" ]
          then
            echo $p
            break
          fi
        done`
        if [ -n "$pid" ]
        then
          kill -15 -$pid $pid 2>/dev/null
          i=0
          while ps -p $pid >/dev/null
          do
            sleep 1
            test $i -lt %2 || break
            i=$((i+1))
          done
          ps -p $pid >/dev/null && kill -9 -$pid $pid 2>/dev/null
          true
        else
          false
        fi)").arg(filePath.path()).arg(globalProjectExplorerSettings().reaperTimeoutInSeconds());
}

MacDevice::MacDevice()
{
    setDisplayType(Tr::tr("Remote macOS"));
    setOsType(OsTypeMac);
    setDefaultDisplayName(Tr::tr("Remote macOS Device"));
    setType(Constants::GenericMacOsType);
    setKillCommandForPathFunction(macKillCommandForPath);
}

namespace Internal {

MacDeviceFactory::MacDeviceFactory()
    : IDeviceFactory(Constants::GenericMacOsType)
{
    setDisplayName(Tr::tr("Remote macOS Device"));
    setIcon(QIcon());
    setQuickCreationAllowed(true);
    setCreator([this]() -> IDevice::Ptr {
        auto device = MacDevice::create();
        m_existingDevices.writeLocked()->push_back(device);

        SshDeviceWizard
            wizard(Tr::tr("New Remote macOS Device Configuration Setup"), IDevice::Ptr(device));
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });

    setConstructionFunction([this] {
        auto device = MacDevice::create();
        m_existingDevices.writeLocked()->push_back(device);
        return device;
    });
    setExecutionTypeId(Constants::ExecutionType);
}

MacDeviceFactory::~MacDeviceFactory()
{
    shutdownExistingDevices();
}

void MacDeviceFactory::shutdownExistingDevices()
{
    m_existingDevices.read([](const std::vector<std::weak_ptr<MacDevice>> &devices) {
        for (auto device : devices) {
            if (auto d = device.lock())
                d->closeConnection(false);
        }
    });
}

} // namespace Internal
} // namespace Remote
