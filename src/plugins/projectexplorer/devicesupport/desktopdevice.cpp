// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"
#include "deviceprocesslist.h"
#include "localprocesslist.h"
#include "desktopprocesssignaloperation.h"

#include <coreplugin/fileutils.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QDateTime>

using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ProjectExplorer {

DesktopDevice::DesktopDevice()
{
    setFileAccess(DesktopDeviceFileAccess::instance());

    setupId(IDevice::AutoDetected, DESKTOP_DEVICE_ID);
    setType(DESKTOP_DEVICE_TYPE);
    setDefaultDisplayName(tr("Local PC"));
    setDisplayType(QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Desktop"));

    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange =
            QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));
    setOpenTerminal([](const Environment &env, const FilePath &workingDir) {
        Core::FileUtils::openTerminal(workingDir, env);
    });
}

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

bool DesktopDevice::canAutoDetectPorts() const
{
    return true;
}

bool DesktopDevice::canCreateProcessModel() const
{
    return true;
}

DeviceProcessList *DesktopDevice::createProcessListModel(QObject *parent) const
{
    return new Internal::LocalProcessList(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

PortsGatheringMethod DesktopDevice::portsGatheringMethod() const
{
    return {
        [this](QAbstractSocket::NetworkLayerProtocol protocol) -> CommandLine {
            // We might encounter the situation that protocol is given IPv6
            // but the consumer of the free port information decides to open
            // an IPv4(only) port. As a result the next IPv6 scan will
            // report the port again as open (in IPv6 namespace), while the
            // same port in IPv4 namespace might still be blocked, and
            // re-use of this port fails.
            // GDBserver behaves exactly like this.

            Q_UNUSED(protocol)

            if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
                return {filePath("netstat"), {"-a", "-n"}};
            if (HostOsInfo::isLinuxHost())
                return {filePath("/bin/sh"), {"-c", "cat /proc/net/tcp*"}};
            return {};
        },

        &Port::parseFromNetstatOutput
    };
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
