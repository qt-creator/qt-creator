// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopprocesssignaloperation.h"
#include "deviceprocesslist.h"
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
    setDefaultDisplayName(Tr::tr("Local PC"));
    setDisplayType(Tr::tr("Desktop"));

    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange
        = QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    setOpenTerminal([](const Environment &env, const FilePath &path) {
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();

        const FilePath shell = Terminal::defaultShellForDevice(path);

        Process process;
        process.setTerminalMode(TerminalMode::Detached);
        process.setEnvironment(realEnv);
        process.setCommand({shell, {}});
        process.setWorkingDirectory(path);
        process.start();
    });
}

DesktopDevice::~DesktopDevice() = default;

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return DeviceInfo();
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

DeviceProcessList *DesktopDevice::createProcessListModel(QObject *parent) const
{
    return new ProcessList(sharedFromThis(), parent);
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

Environment DesktopDevice::systemEnvironment() const
{
    return Environment::systemEnvironment();
}

FilePath DesktopDevice::rootPath() const
{
    if (id() == DESKTOP_DEVICE_ID)
        return FilePath::fromParts({}, {}, QDir::rootPath());
    return IDevice::rootPath();
}

} // namespace ProjectExplorer
