/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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
#include "qnxdevicetester.h"
#include "qnxdeviceprocesslist.h"
#include "qnxdeviceprocesssignaloperation.h"
#include "qnxdeployqtlibrariesdialog.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QRegExp>
#include <QStringList>
#include <QThread>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char QnxVersionKey[] = "QnxVersion";
const char DeployQtLibrariesActionId [] = "Qnx.Qnx.DeployQtLibrariesAction";
}

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
    ProjectExplorer::SshDeviceProcess versionNumberProcess(sharedFromThis());
    QObject::connect(&versionNumberProcess, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QObject::connect(&versionNumberProcess, SIGNAL(error(QProcess::ProcessError)), &eventLoop, SLOT(quit()));

    QStringList arguments;
    arguments << QLatin1String("-r");
    versionNumberProcess.start(QLatin1String("uname"), arguments);

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

ProjectExplorer::IDevice::Ptr QnxDeviceConfiguration::clone() const
{
    return Ptr(new QnxDeviceConfiguration(*this));
}

ProjectExplorer::PortsGatheringMethod::Ptr QnxDeviceConfiguration::portsGatheringMethod() const
{
    return ProjectExplorer::PortsGatheringMethod::Ptr(new QnxPortsGatheringMethod);
}

ProjectExplorer::DeviceProcessList *QnxDeviceConfiguration::createProcessListModel(QObject *parent) const
{
    return new QnxDeviceProcessList(sharedFromThis(), parent);
}

ProjectExplorer::DeviceTester *QnxDeviceConfiguration::createDeviceTester() const
{
    return new QnxDeviceTester;
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
        QnxDeployQtLibrariesDialog dialog(device, QnxDeployQtLibrariesDialog::QNX, parent);
        dialog.exec();
    } else {
        RemoteLinux::LinuxDevice::executeAction(actionId, parent);
    }
}

ProjectExplorer::DeviceProcessSignalOperation::Ptr QnxDeviceConfiguration::signalOperation() const
{
    return ProjectExplorer::DeviceProcessSignalOperation::Ptr(
                new QnxDeviceProcessSignalOperation(sshParameters()));
}
