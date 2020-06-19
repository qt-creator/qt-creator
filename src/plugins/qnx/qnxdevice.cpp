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

#include "qnxdevice.h"

#include "qnxconstants.h"
#include "qnxdevicetester.h"
#include "qnxdeviceprocesslist.h"
#include "qnxdeviceprocesssignaloperation.h"
#include "qnxdeployqtlibrariesdialog.h"
#include "qnxdeviceprocess.h"
#include "qnxdevicewizard.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <projectexplorer/runcontrol.h>

#include <ssh/sshconnection.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

const char QnxVersionKey[] = "QnxVersion";

class QnxPortsGatheringMethod : public PortsGatheringMethod
{
    // TODO: The command is probably needlessly complicated because the parsing method
    // used to be fixed. These two can now be matched to each other.
    Runnable runnable(QAbstractSocket::NetworkLayerProtocol protocol) const override
    {
        Q_UNUSED(protocol)
        Runnable runnable;
        runnable.executable = FilePath::fromString("netstat");
        runnable.commandLineArguments = "-na";
        return runnable;
    }

    QList<Port> usedPorts(const QByteArray &output) const override
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

QnxDevice::QnxDevice()
{
    setDisplayType(tr("QNX"));
    setDefaultDisplayName(tr("QNX Device"));
    setOsType(OsTypeOtherUnix);

    addDeviceAction({tr("Deploy Qt libraries..."), [](const IDevice::Ptr &device, QWidget *parent) {
        QnxDeployQtLibrariesDialog dialog(device, parent);
        dialog.exec();
    }});
}

int QnxDevice::qnxVersion() const
{
    if (m_versionNumber == 0)
        updateVersionNumber();

    return m_versionNumber;
}

void QnxDevice::updateVersionNumber() const
{
    QEventLoop eventLoop;
    SshDeviceProcess versionNumberProcess(sharedFromThis());
    QObject::connect(&versionNumberProcess, &SshDeviceProcess::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(&versionNumberProcess, &DeviceProcess::error, &eventLoop, &QEventLoop::quit);

    Runnable r;
    r.executable = FilePath::fromString("uname");
    r.commandLineArguments = QLatin1String("-r");
    versionNumberProcess.start(r);

    bool isGuiThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    if (isGuiThread)
        QApplication::setOverrideCursor(Qt::WaitCursor);

    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    QByteArray output = versionNumberProcess.readAllStandardOutput();
    QString versionMessage = QString::fromLatin1(output);
    const QRegularExpression versionNumberRegExp("(\\d+)\\.(\\d+)\\.(\\d+)");
    const QRegularExpressionMatch match = versionNumberRegExp.match(versionMessage);
    if (match.hasMatch()) {
        int major = match.captured(1).toInt();
        int minor = match.captured(2).toInt();
        int patch = match.captured(3).toInt();
        m_versionNumber = (major << 16)|(minor<<8)|(patch);
    }

    if (isGuiThread)
        QApplication::restoreOverrideCursor();
}

void QnxDevice::fromMap(const QVariantMap &map)
{
    m_versionNumber = map.value(QLatin1String(QnxVersionKey), 0).toInt();
    RemoteLinux::LinuxDevice::fromMap(map);
}

QVariantMap QnxDevice::toMap() const
{
    QVariantMap map(RemoteLinux::LinuxDevice::toMap());
    map.insert(QLatin1String(QnxVersionKey), m_versionNumber);
    return map;
}

PortsGatheringMethod::Ptr QnxDevice::portsGatheringMethod() const
{
    return PortsGatheringMethod::Ptr(new QnxPortsGatheringMethod);
}

DeviceProcessList *QnxDevice::createProcessListModel(QObject *parent) const
{
    return new QnxDeviceProcessList(sharedFromThis(), parent);
}

DeviceTester *QnxDevice::createDeviceTester() const
{
    return new QnxDeviceTester;
}

DeviceProcess *QnxDevice::createProcess(QObject *parent) const
{
    return new QnxDeviceProcess(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr QnxDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(
                new QnxDeviceProcessSignalOperation(sshParameters()));
}

// Factory

QnxDeviceFactory::QnxDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::QNX_QNX_OS_TYPE)
{
    setDisplayName(QnxDevice::tr("QNX Device"));
    setCombinedIcon(":/qnx/images/qnxdevicesmall.png",
                    ":/qnx/images/qnxdevice.png");
    setCanCreate(true);
    setConstructionFunction(&QnxDevice::create);
}

ProjectExplorer::IDevice::Ptr QnxDeviceFactory::create() const
{
    QnxDeviceWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return ProjectExplorer::IDevice::Ptr();
    return wizard.device();
}

} // namespace Internal
} // namespace Qnx
