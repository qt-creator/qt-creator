/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxdeviceconfiguration.h"

using namespace Qnx;
using namespace Qnx::Internal;

class QnxDeviceProcessSupport : public RemoteLinux::LinuxDeviceProcessSupport
{
    QString killProcessByNameCommandLine(const QString &filePath) const
    {
        QString executable = filePath;
        return QString::fromLatin1("for PID in $(ps -f -o pid,comm | grep %1 | awk '/%1/ {print $1}'); "
            "do "
                "kill $PID; sleep 1; kill -9 $PID; "
            "done").arg(executable.replace(QLatin1String("/"), QLatin1String("\\/")));
    }
};


class QnxPortsGatheringMethod : public ProjectExplorer::PortsGatheringMethod
{
    // TODO: The command is probably needlessly complicated because the parsing method
    // used to be fixed. These two can now be matched to each other.
    QByteArray commandLine(QAbstractSocket::NetworkLayerProtocol protocol) const
    {
        Q_UNUSED(protocol);
        return "netstat -na "
                "| sed 's/[a-z]\\+\\s\\+[0-9]\\+\\s\\+[0-9]\\+\\s\\+\\(\\*\\|[0-9\\.]\\+\\)\\.\\([0-9]\\+\\).*/\\2/g' "
                "| while read line; do "
                    "if [[ $line != udp* ]] && [[ $line != Active* ]]; then "
                        "printf '%x\n' $line; "
                    "fi; "
                "done";
    }

    QList<int> usedPorts(const QByteArray &output) const
    {
        QList<int> ports;
        QList<QByteArray> portStrings = output.split('\n');
        portStrings.removeFirst();
        foreach (const QByteArray &portString, portStrings) {
            if (portString.isEmpty())
                continue;
            bool ok;
            const int port = portString.toInt(&ok, 16);
            if (ok) {
                if (!ports.contains(port))
                    ports << port;
            } else {
                qWarning("%s: Unexpected string '%s' is not a port.",
                         Q_FUNC_INFO, portString.data());
            }
        }
        return ports;
    }
};

QnxDeviceConfiguration::QnxDeviceConfiguration()
    : RemoteLinux::LinuxDevice()
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QnxDeviceConfiguration &other)
    : RemoteLinux::LinuxDevice(other)
{
}


QnxDeviceConfiguration::Ptr QnxDeviceConfiguration::create()
{
    return Ptr(new QnxDeviceConfiguration);
}

QnxDeviceConfiguration::Ptr QnxDeviceConfiguration::create(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new QnxDeviceConfiguration(name, type, machineType, origin, id));
}

QString QnxDeviceConfiguration::displayType() const
{
    return tr("QNX");
}

ProjectExplorer::IDevice::Ptr QnxDeviceConfiguration::clone() const
{
    return Ptr(new QnxDeviceConfiguration(*this));
}

ProjectExplorer::DeviceProcessSupport::Ptr QnxDeviceConfiguration::processSupport() const
{
    return ProjectExplorer::DeviceProcessSupport::Ptr(new QnxDeviceProcessSupport);
}

ProjectExplorer::PortsGatheringMethod::Ptr QnxDeviceConfiguration::portsGatheringMethod() const
{
    return ProjectExplorer::PortsGatheringMethod::Ptr(new QnxPortsGatheringMethod);
}
