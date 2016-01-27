/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeviceconfiguration.h"
#include "qnxdevicetester.h"
#include "qnxdeviceprocesslist.h"
#include "qnxdeviceprocesssignaloperation.h"
#include "qnxdeployqtlibrariesdialog.h"
#include "qnxdeviceprocess.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <projectexplorer/runnables.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QRegExp>
#include <QStringList>
#include <QThread>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {

using namespace Internal;

const char QnxVersionKey[] = "QnxVersion";
const char DeployQtLibrariesActionId [] = "Qnx.Qnx.DeployQtLibrariesAction";

class QnxPortsGatheringMethod : public PortsGatheringMethod
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
    , m_versionNumber(0)
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
    , m_versionNumber(0)
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QnxDeviceConfiguration &other)
    : RemoteLinux::LinuxDevice(other)
    , m_versionNumber(other.m_versionNumber)
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

int QnxDeviceConfiguration::qnxVersion() const
{
    if (m_versionNumber == 0)
        updateVersionNumber();

    return m_versionNumber;
}

void QnxDeviceConfiguration::updateVersionNumber() const
{
    QEventLoop eventLoop;
    SshDeviceProcess versionNumberProcess(sharedFromThis());
    QObject::connect(&versionNumberProcess, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QObject::connect(&versionNumberProcess, SIGNAL(error(QProcess::ProcessError)), &eventLoop, SLOT(quit()));

    StandardRunnable r;
    r.executable = QLatin1String("uname");
    r.commandLineArguments = QLatin1String("-r");
    versionNumberProcess.start(r);

    bool isGuiThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread)
        QApplication::setOverrideCursor(Qt::WaitCursor);

    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    QByteArray output = versionNumberProcess.readAllStandardOutput();
    QString versionMessage = QString::fromLatin1(output);
    QRegExp versionNumberRegExp = QRegExp(QLatin1String("(\\d+)\\.(\\d+)\\.(\\d+)"));
    if (versionNumberRegExp.indexIn(versionMessage) > -1 && versionNumberRegExp.captureCount() == 3) {
        int major = versionNumberRegExp.cap(1).toInt();
        int minor = versionNumberRegExp.cap(2).toInt();
        int patch = versionNumberRegExp.cap(3).toInt();
        m_versionNumber = (major << 16)|(minor<<8)|(patch);
    }

    if (isGuiThread)
        QApplication::restoreOverrideCursor();
}

void QnxDeviceConfiguration::fromMap(const QVariantMap &map)
{
    m_versionNumber = map.value(QLatin1String(QnxVersionKey), 0).toInt();
    RemoteLinux::LinuxDevice::fromMap(map);
}

QVariantMap QnxDeviceConfiguration::toMap() const
{
    QVariantMap map(RemoteLinux::LinuxDevice::toMap());
    map.insert(QLatin1String(QnxVersionKey), m_versionNumber);
    return map;
}

IDevice::Ptr QnxDeviceConfiguration::clone() const
{
    return Ptr(new QnxDeviceConfiguration(*this));
}

PortsGatheringMethod::Ptr QnxDeviceConfiguration::portsGatheringMethod() const
{
    return PortsGatheringMethod::Ptr(new QnxPortsGatheringMethod);
}

DeviceProcessList *QnxDeviceConfiguration::createProcessListModel(QObject *parent) const
{
    return new QnxDeviceProcessList(sharedFromThis(), parent);
}

DeviceTester *QnxDeviceConfiguration::createDeviceTester() const
{
    return new QnxDeviceTester;
}

DeviceProcess *QnxDeviceConfiguration::createProcess(QObject *parent) const
{
    return new QnxDeviceProcess(sharedFromThis(), parent);
}

QList<Core::Id> QnxDeviceConfiguration::actionIds() const
{
    QList<Core::Id> actions = RemoteLinux::LinuxDevice::actionIds();
    actions << Core::Id(DeployQtLibrariesActionId);
    return actions;
}

QString QnxDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    if (actionId == Core::Id(DeployQtLibrariesActionId))
        return tr("Deploy Qt libraries...");

    return RemoteLinux::LinuxDevice::displayNameForActionId(actionId);
}

void QnxDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent)
{
    const QnxDeviceConfiguration::ConstPtr device =
            sharedFromThis().staticCast<const QnxDeviceConfiguration>();
    if (actionId == Core::Id(DeployQtLibrariesActionId)) {
        QnxDeployQtLibrariesDialog dialog(device, parent);
        dialog.exec();
    } else {
        RemoteLinux::LinuxDevice::executeAction(actionId, parent);
    }
}

DeviceProcessSignalOperation::Ptr QnxDeviceConfiguration::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(
                new QnxDeviceProcessSignalOperation(sshParameters()));
}

} // namespace Qnx
