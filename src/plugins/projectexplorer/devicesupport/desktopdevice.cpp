/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "desktopdevice.h"
#include "desktopdeviceprocess.h"
#include "deviceprocesslist.h"
#include "localprocesslist.h"
#include "desktopdeviceconfigurationwidget.h"
#include "desktopprocesssignaloperation.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>

#include <ssh/sshconnection.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/stringutils.h>
#include <utils/url.h>

#include <QCoreApplication>

using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ProjectExplorer {

DesktopDevice::DesktopDevice() : IDevice(Core::Id(DESKTOP_DEVICE_TYPE),
                                         IDevice::AutoDetected,
                                         IDevice::Hardware,
                                         Core::Id(DESKTOP_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Local PC"));
    setDeviceState(IDevice::DeviceStateUnknown);
    const QString portRange =
            QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));
}

DesktopDevice::DesktopDevice(const DesktopDevice &other) :
    IDevice(other)
{ }

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return DeviceInfo();
}

QString DesktopDevice::displayType() const
{
    return QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Desktop");
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return 0;
    // DesktopDeviceConfigurationWidget currently has just one editable field viz. free ports.
    // Querying for an available port is quite straightforward. Having a field for the port
    // range can be confusing to the user. Hence, disabling the widget for now.
}

QList<Core::Id> DesktopDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString DesktopDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void DesktopDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
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

DeviceProcess *DesktopDevice::createProcess(QObject *parent) const
{
    return new Internal::DesktopDeviceProcess(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

class DesktopDeviceEnvironmentFetcher : public DeviceEnvironmentFetcher
{
public:
    DesktopDeviceEnvironmentFetcher() {}

    void start() override
    {
        emit finished(Utils::Environment::systemEnvironment(), true);
    }
};

DeviceEnvironmentFetcher::Ptr DesktopDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr(new DesktopDeviceEnvironmentFetcher());
}

class DesktopPortsGatheringMethod : public PortsGatheringMethod
{
    Runnable runnable(QAbstractSocket::NetworkLayerProtocol protocol) const override
    {
        // We might encounter the situation that protocol is given IPv6
        // but the consumer of the free port information decides to open
        // an IPv4(only) port. As a result the next IPv6 scan will
        // report the port again as open (in IPv6 namespace), while the
        // same port in IPv4 namespace might still be blocked, and
        // re-use of this port fails.
        // GDBserver behaves exactly like this.

        Q_UNUSED(protocol)

        StandardRunnable runnable;
        if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost()) {
            runnable.executable = "netstat";
            runnable.commandLineArguments =  "-a -n";
        } else if (HostOsInfo::isLinuxHost()) {
            runnable.executable = "/bin/sh";
            runnable.commandLineArguments = "-c 'cat /proc/net/tcp*'";
        }
        return runnable;
    }

    QList<Utils::Port> usedPorts(const QByteArray &output) const override
    {
        QList<Utils::Port> ports;
        const QList<QByteArray> lines = output.split('\n');
        for (const QByteArray &line : lines) {
            const Port port(Utils::parseUsedPortFromNetstatOutput(line));
            if (port.isValid() && !ports.contains(port))
                ports.append(port);
        }
        return ports;
    }
};

PortsGatheringMethod::Ptr DesktopDevice::portsGatheringMethod() const
{
    return DesktopPortsGatheringMethod::Ptr(new DesktopPortsGatheringMethod);
}

QUrl DesktopDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}

Utils::OsType DesktopDevice::osType() const
{
    return Utils::HostOsInfo::hostOs();
}

IDevice::Ptr DesktopDevice::clone() const
{
    return Ptr(new DesktopDevice(*this));
}

} // namespace ProjectExplorer
