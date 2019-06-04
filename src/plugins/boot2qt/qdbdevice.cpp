/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbdevice.h"

#include "qdbutils.h"
#include "deviceapplicationobserver.h"
#include "qdbconstants.h"
#include "qdbdeviceprocess.h"
#include "qdbdevicedebugsupport.h"
#include "qdbdevicewizard.h"

#include <coreplugin/icore.h>

#include <ssh/sshconnection.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

QdbDevice::QdbDevice()
{
    addDeviceAction({tr("Reboot Device"), [](const IDevice::Ptr &device, QWidget *) {
        QList<Command> commands{Command("reboot")};
        (new DeviceApplicationObserver)->start(device, commands);
    }});

    addDeviceAction({tr("Restore Default App"), [](const IDevice::Ptr &device, QWidget *) {
        QList<Command> commands{Command(appControllerFilePath(), QStringList{"--remove-default"})};
        (new DeviceApplicationObserver)->start(device, commands);
    }});
}

QString QdbDevice::displayType() const
{
    return tr("Boot2Qt Device");
}

ProjectExplorer::IDeviceWidget *QdbDevice::createWidget()
{
    ProjectExplorer::IDeviceWidget *w = RemoteLinux::LinuxDevice::createWidget();

    return w;
}

ProjectExplorer::DeviceProcess *QdbDevice::createProcess(QObject *parent) const
{
    return new QdbDeviceProcess(sharedFromThis(), parent);
}

void QdbDevice::setSerialNumber(const QString &serial)
{
    m_serialNumber = serial;
}

QString QdbDevice::serialNumber() const
{
    return m_serialNumber;
}

void QdbDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    setSerialNumber(map.value("Qdb.SerialNumber").toString());
}

QVariantMap QdbDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert("Qdb.SerialNumber", serialNumber());
    return map;
}

void QdbDevice::setupDefaultNetworkSettings(const QString &host)
{
    setFreePorts(Utils::PortList::fromString("10000-10100"));

    QSsh::SshConnectionParameters parameters = sshParameters();
    parameters.setHost(host);
    parameters.setUserName("root");
    parameters.setPort(22);
    parameters.timeout = 10;
    parameters.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypeAll;
    setSshParameters(parameters);
}

std::function<ProjectExplorer::RunWorker *(ProjectExplorer::RunControl *)>
    QdbDevice::workerCreator(Core::Id id) const
{
    if (id == "PerfRecorder") {
        return [](ProjectExplorer::RunControl *runControl) {
            return new QdbDevicePerfProfilerSupport(runControl);
        };
    }
    return {};
}


// Device factory

static IDevice::Ptr createDevice(QdbDeviceWizard::DeviceType deviceType)
{
    QdbDeviceWizard wizard(deviceType, Core::ICore::mainWindow());

    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

QdbLinuxDeviceFactory::QdbLinuxDeviceFactory()
    : IDeviceFactory(Constants::QdbLinuxOsType)
{
    setDisplayName(tr("Boot2Qt Device"));
    setCombinedIcon(":/qdb/images/qdbdevicesmall.png", ":/qdb/images/qdbdevice.png");
    setCanCreate(true);
    setConstructionFunction(&QdbDevice::create);
}

IDevice::Ptr QdbLinuxDeviceFactory::create() const
{
    return createDevice(QdbDeviceWizard::HardwareDevice);
}

} // namespace Internal
} // namespace Qdb
