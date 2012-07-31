/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "linuxdeviceconfiguration.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinux_constants.h"

#include <coreplugin/id.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
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

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QString &name,
       Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new LinuxDeviceConfiguration(name, type, machineType, origin, id));
}

QString LinuxDeviceConfiguration::displayType() const
{
    return tr("Generic Linux");
}

ProjectExplorer::IDeviceWidget *LinuxDeviceConfiguration::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> LinuxDeviceConfiguration::actionIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericTestDeviceActionId)
        << Core::Id(Constants::GenericDeployKeyToDeviceActionId)
        << Core::Id(Constants::GenericRemoteProcessesActionId);
}

QString LinuxDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        return tr("Test");
    if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        return tr("Remote Processes...");
    if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

void LinuxDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    QDialog *d = 0;
    const LinuxDeviceConfiguration::ConstPtr device
            = sharedFromThis().staticCast<const LinuxDeviceConfiguration>();
    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        d = new LinuxDeviceTestDialog(device, new GenericLinuxDeviceTester, parent);
    else if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        d = new DeviceProcessesDialog(new DeviceProcessList(device, parent));
    else if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        d = PublicKeyDeploymentDialog::createDialog(device, parent);
    if (d)
        d->exec();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType,
        Origin origin, Core::Id id)
    : IDevice(type, origin, machineType, id)
{
    setDisplayName(name);
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration &other)
    : IDevice(other)
{
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create()
{
    return Ptr(new LinuxDeviceConfiguration);
}

ProjectExplorer::IDevice::Ptr LinuxDeviceConfiguration::clone() const
{
    return Ptr(new LinuxDeviceConfiguration(*this));
}

QString LinuxDeviceConfiguration::listProcessesCommandLine() const
{
    return QString::fromLatin1(
        "for dir in `ls -d /proc/[0123456789]*`; do "
            "test -d $dir || continue;" // Decrease the likelihood of a race condition.
            "echo $dir;"
            "cat $dir/cmdline;echo;" // cmdline does not end in newline
            "cat $dir/stat;"
            "readlink $dir/exe;"
            "printf '%1''%2';"
        "done").arg(Delimiter0).arg(Delimiter1);
}

QString LinuxDeviceConfiguration::killProcessCommandLine(const DeviceProcess &process) const
{
    return QLatin1String("kill -9 ") + QString::number(process.pid);
}

QList<DeviceProcess> LinuxDeviceConfiguration::buildProcessList(const QString &listProcessesReply) const
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

} // namespace RemoteLinux
