// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopprocesssignaloperation.h"
#include "processlist.h"

#include <coreplugin/fileutils.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/terminalcommand.h>
#include <utils/terminalhooks.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <cstring>
#include <stdlib.h>
#include <windows.h>
#endif

using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ProjectExplorer {

class DesktopDevicePrivate : public QObject
{};

DesktopDevice::DesktopDevice()
    : d(new DesktopDevicePrivate())
{
    setFileAccess(DesktopDeviceFileAccess::instance());

    setupId(IDevice::AutoDetected, DESKTOP_DEVICE_ID);
    setType(DESKTOP_DEVICE_TYPE);
    settings()->displayName.setDefaultValue(Tr::tr("Local PC"));
    setDisplayType(Tr::tr("Desktop"));

    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange
        = QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    setOpenTerminal([](const Environment &env, const FilePath &path) -> expected_str<void> {
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();

        const expected_str<FilePath> shell = Terminal::defaultShellForDevice(path);
        if (!shell)
            return make_unexpected(shell.error());

        Process process;
        process.setTerminalMode(TerminalMode::Detached);
        process.setEnvironment(realEnv);
        process.setCommand({*shell, {}});
        process.setWorkingDirectory(path);
        process.start();

        return {};
    });
}

DesktopDevice::~DesktopDevice() = default;

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return {};
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return nullptr;
    // DesktopDeviceConfigurationWidget currently has just one editable field viz. free ports.
    // Querying for an available port is quite straightforward. Having a field for the port
    // range can be confusing to the user. Hence, disabling the widget for now.
}

bool DesktopDevice::canCreateProcessModel() const
{
    return true;
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

QUrl DesktopDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}

bool DesktopDevice::usableAsBuildDevice() const
{
    return true;
}

bool DesktopDevice::handlesFile(const FilePath &filePath) const
{
    return !filePath.needsDevice();
}

FilePath DesktopDevice::filePath(const QString &pathOnDevice) const
{
    return FilePath::fromParts({}, {}, pathOnDevice);
}

expected_str<Environment> DesktopDevice::systemEnvironmentWithError() const
{
    return Environment::systemEnvironment();
}

FilePath DesktopDevice::rootPath() const
{
    if (id() == DESKTOP_DEVICE_ID)
        return HostOsInfo::root();
    return IDevice::rootPath();
}

void DesktopDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    const FilePath rsync = FilePath::fromString("rsync").searchInPath();
    const FilePath sftp = FilePath::fromString("sftp").searchInPath();
    setExtraData(Constants::SUPPORTS_RSYNC, rsync.isExecutableFile());
    setExtraData(Constants::SUPPORTS_SFTP, sftp.isExecutableFile());
}

} // namespace ProjectExplorer
