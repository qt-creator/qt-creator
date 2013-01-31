/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "linuxdevice.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"

#include <coreplugin/id.h>
#include <projectexplorer/devicesupport/sshdeviceprocesslist.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

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

    QList<DeviceProcess> buildProcessList(const QString &listProcessesReply) const
    {
        QList<DeviceProcess> processes;
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
            const int pid = elements.first().mid(6).toInt(&ok);
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

            DeviceProcess process;
            process.pid = pid;
            process.cmdLine = command;
            process.exe = elements.at(3);
            processes.append(process);
        }

        qSort(processes);
        return processes;
    }
};


QString LinuxDeviceProcessSupport::killProcessByPidCommandLine(int pid) const
{
    return QLatin1String("kill -9 ") + QString::number(pid);
}

QString LinuxDeviceProcessSupport::killProcessByNameCommandLine(const QString &filePath) const
{
    return QString::fromLatin1("cd /proc; for pid in `ls -d [0123456789]*`; "
            "do "
                "if [ \"`readlink /proc/$pid/exe`\" = \"%1\" ]; then "
                "    kill $pid; sleep 1; kill -9 $pid; "
                "fi; "
            "done").arg(filePath);
}


class LinuxPortsGatheringMethod : public ProjectExplorer::PortsGatheringMethod
{
    QByteArray commandLine(QAbstractSocket::NetworkLayerProtocol protocol) const
    {
        QString procFilePath;
        int addressLength;
        if (protocol == QAbstractSocket::IPv4Protocol) {
            procFilePath = QLatin1String("/proc/net/tcp");
            addressLength = 8;
        } else {
            procFilePath = QLatin1String("/proc/net/tcp6");
            addressLength = 32;
        }
        return QString::fromLatin1("sed "
                "'s/.*: [[:xdigit:]]\\{%1\\}:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' %2")
                .arg(addressLength).arg(procFilePath).toUtf8();
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


LinuxDevice::Ptr LinuxDevice::create(const QString &name,
       Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new LinuxDevice(name, type, machineType, origin, id));
}

QString LinuxDevice::displayType() const
{
    return tr("Generic Linux");
}

ProjectExplorer::IDeviceWidget *LinuxDevice::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> LinuxDevice::actionIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericTestDeviceActionId)
            << Core::Id(Constants::GenericDeployKeyToDeviceActionId);
}

QString LinuxDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::GenericTestDeviceActionId)
        return tr("Test");
    if (actionId == Constants::GenericDeployKeyToDeviceActionId)
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

void LinuxDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    QDialog *d = 0;
    const LinuxDevice::ConstPtr device = sharedFromThis().staticCast<const LinuxDevice>();
    if (actionId == Constants::GenericTestDeviceActionId)
        d = new LinuxDeviceTestDialog(device, new GenericLinuxDeviceTester, parent);
    else if (actionId == Constants::GenericDeployKeyToDeviceActionId)
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

ProjectExplorer::IDevice::Ptr LinuxDevice::clone() const
{
    return Ptr(new LinuxDevice(*this));
}

DeviceProcessSupport::Ptr LinuxDevice::processSupport() const
{
    return DeviceProcessSupport::Ptr(new LinuxDeviceProcessSupport);
}

PortsGatheringMethod::Ptr LinuxDevice::portsGatheringMethod() const
{
    return LinuxPortsGatheringMethod::Ptr(new LinuxPortsGatheringMethod);
}

DeviceProcessList *LinuxDevice::createProcessListModel(QObject *parent) const
{
    return new LinuxDeviceProcessList(sharedFromThis(), parent);
}

} // namespace RemoteLinux
