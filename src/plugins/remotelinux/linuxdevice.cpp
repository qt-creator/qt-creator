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

#include "linuxdevice.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdeviceprocess.h"
#include "linuxdevicetester.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"
#include "remotelinuxsignaloperation.h"
#include "remotelinuxenvironmentreader.h"

#include <coreplugin/id.h>
#include <projectexplorer/devicesupport/sshdeviceprocesslist.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/algorithm.h>
#include <utils/port.h>
#include <utils/qtcassert.h>

#include <QTimer>

using namespace ProjectExplorer;

namespace RemoteLinux {

const char Delimiter0[] = "x--";
const char Delimiter1[] = "---";

static QString visualizeNull(QString s)
{
    return s.replace(QLatin1Char('\0'), QLatin1String("<null>"));
}

class LinuxDeviceProcessList : public SshDeviceProcessList
{
public:
    LinuxDeviceProcessList(const IDevice::ConstPtr &device, QObject *parent)
            : SshDeviceProcessList(device, parent)
    {
    }

private:
    QString listProcessesCommandLine() const
    {
        return QString::fromLatin1(
            "for dir in `ls -d /proc/[0123456789]*`; do "
                "test -d $dir || continue;" // Decrease the likelihood of a race condition.
                "echo $dir;"
                "cat $dir/cmdline;echo;" // cmdline does not end in newline
                "cat $dir/stat;"
                "readlink $dir/exe;"
                "printf '%1''%2';"
            "done").arg(QLatin1String(Delimiter0)).arg(QLatin1String(Delimiter1));
    }

    QList<DeviceProcessItem> buildProcessList(const QString &listProcessesReply) const
    {
        QList<DeviceProcessItem> processes;
        const QStringList lines = listProcessesReply.split(QString::fromLatin1(Delimiter0)
                + QString::fromLatin1(Delimiter1), QString::SkipEmptyParts);
        foreach (const QString &line, lines) {
            const QStringList elements = line.split(QLatin1Char('\n'));
            if (elements.count() < 4) {
                qDebug("%s: Expected four list elements, got %d. Line was '%s'.", Q_FUNC_INFO,
                       elements.count(), qPrintable(visualizeNull(line)));
                continue;
            }
            bool ok;
            const int pid = elements.first().midRef(6).toInt(&ok);
            if (!ok) {
                qDebug("%s: Expected number in %s. Line was '%s'.", Q_FUNC_INFO,
                       qPrintable(elements.first()), qPrintable(visualizeNull(line)));
                continue;
            }
            QString command = elements.at(1);
            command.replace(QLatin1Char('\0'), QLatin1Char(' '));
            if (command.isEmpty()) {
                const QString &statString = elements.at(2);
                const int openParenPos = statString.indexOf(QLatin1Char('('));
                const int closedParenPos = statString.indexOf(QLatin1Char(')'), openParenPos);
                if (openParenPos == -1 || closedParenPos == -1)
                    continue;
                command = QLatin1Char('[')
                        + statString.mid(openParenPos + 1, closedParenPos - openParenPos - 1)
                        + QLatin1Char(']');
            }

            DeviceProcessItem process;
            process.pid = pid;
            process.cmdLine = command;
            process.exe = elements.at(3);
            processes.append(process);
        }

        Utils::sort(processes);
        return processes;
    }
};

class LinuxPortsGatheringMethod : public PortsGatheringMethod
{
    QByteArray commandLine(QAbstractSocket::NetworkLayerProtocol protocol) const
    {
        // We might encounter the situation that protocol is given IPv6
        // but the consumer of the free port information decides to open
        // an IPv4(only) port. As a result the next IPv6 scan will
        // report the port again as open (in IPv6 namespace), while the
        // same port in IPv4 namespace might still be blocked, and
        // re-use of this port fails.
        // GDBserver behaves exactly like this.

        Q_UNUSED(protocol)

        // /proc/net/tcp* covers /proc/net/tcp and /proc/net/tcp6
        return "sed -e 's/.*: [[:xdigit:]]*:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp*";
    }

    QList<Utils::Port> usedPorts(const QByteArray &output) const
    {
        QList<Utils::Port> ports;
        QList<QByteArray> portStrings = output.split('\n');
        foreach (const QByteArray &portString, portStrings) {
            if (portString.size() != 4)
                continue;
            bool ok;
            const Utils::Port port(portString.toInt(&ok, 16));
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


LinuxDevice::Ptr LinuxDevice::create(const QString &name,
       Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new LinuxDevice(name, type, machineType, origin, id));
}

QString LinuxDevice::displayType() const
{
    return tr("Generic Linux");
}

IDeviceWidget *LinuxDevice::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> LinuxDevice::actionIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericDeployKeyToDeviceActionId);
}

QString LinuxDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::GenericDeployKeyToDeviceActionId)
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

void LinuxDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    QDialog *d = 0;
    const LinuxDevice::ConstPtr device = sharedFromThis().staticCast<const LinuxDevice>();
    if (actionId == Constants::GenericDeployKeyToDeviceActionId)
        d = PublicKeyDeploymentDialog::createDialog(device, parent);
    if (d)
        d->exec();
    delete d;
}

LinuxDevice::LinuxDevice(const QString &name, Core::Id type, MachineType machineType,
        Origin origin, Core::Id id)
    : IDevice(type, origin, machineType, id)
{
    setDisplayName(name);
}

LinuxDevice::LinuxDevice(const LinuxDevice &other)
    : IDevice(other)
{
}

LinuxDevice::Ptr LinuxDevice::create()
{
    return Ptr(new LinuxDevice);
}

IDevice::Ptr LinuxDevice::clone() const
{
    return Ptr(new LinuxDevice(*this));
}

DeviceProcess *LinuxDevice::createProcess(QObject *parent) const
{
    return new LinuxDeviceProcess(sharedFromThis(), parent);
}

bool LinuxDevice::canAutoDetectPorts() const
{
    return true;
}

PortsGatheringMethod::Ptr LinuxDevice::portsGatheringMethod() const
{
    return LinuxPortsGatheringMethod::Ptr(new LinuxPortsGatheringMethod);
}

DeviceProcessList *LinuxDevice::createProcessListModel(QObject *parent) const
{
    return new LinuxDeviceProcessList(sharedFromThis(), parent);
}

DeviceTester *LinuxDevice::createDeviceTester() const
{
    return new GenericLinuxDeviceTester;
}

DeviceProcessSignalOperation::Ptr LinuxDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new RemoteLinuxSignalOperation(sshParameters()));
}

class LinuxDeviceEnvironmentFetcher : public DeviceEnvironmentFetcher
{
public:
    LinuxDeviceEnvironmentFetcher(const IDevice::ConstPtr &device)
        : m_reader(device)
    {
        connect(&m_reader, &Internal::RemoteLinuxEnvironmentReader::finished,
                this, &LinuxDeviceEnvironmentFetcher::readerFinished);
        connect(&m_reader, &Internal::RemoteLinuxEnvironmentReader::error,
                this, &LinuxDeviceEnvironmentFetcher::readerError);
    }

private:
    void start() override { m_reader.start(); }
    void readerFinished() { emit finished(m_reader.remoteEnvironment(), true); }
    void readerError() { emit finished(Utils::Environment(), false); }

    Internal::RemoteLinuxEnvironmentReader m_reader;
};

DeviceEnvironmentFetcher::Ptr LinuxDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr(new LinuxDeviceEnvironmentFetcher(sharedFromThis()));
}

} // namespace RemoteLinux
