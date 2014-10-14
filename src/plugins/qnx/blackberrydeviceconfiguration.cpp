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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydeviceconfiguration.h"

#include "qnxconstants.h"
#include "qnxdeviceprocesssignaloperation.h"
#include "qnxdeployqtlibrariesdialog.h"
#include "blackberrydeviceconfigurationwidget.h"
#include "blackberrydeviceconnectionmanager.h"
#include "qnxdeviceprocesslist.h"

#include <projectexplorer/kitinformation.h>
#include <ssh/sshconnection.h>

using namespace Qnx;
using namespace Qnx::Internal;
using namespace ProjectExplorer;

namespace {
const char ConnectToDeviceActionId[]      = "Qnx.BlackBerry.ConnectToDeviceAction";
const char DisconnectFromDeviceActionId[] = "Qnx.BlackBerry.DisconnectFromDeviceAction";
const char DeployQtLibrariesActionId[]    = "Qnx.BlackBerry.DeployQtLibrariesAction";
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration()
    : RemoteLinux::LinuxDevice()
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const QString &name, Core::Id type,
                                                             IDevice::MachineType machineType,
                                                             IDevice::Origin origin, Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const BlackBerryDeviceConfiguration &other)
    : RemoteLinux::LinuxDevice(other)
    , m_debugToken(other.m_debugToken)
{
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create()
{
    return Ptr(new BlackBerryDeviceConfiguration);
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create(const QString &name, Core::Id type,
                                                                         IDevice::MachineType machineType,
                                                                         IDevice::Origin origin, Core::Id id)
{
    return Ptr(new BlackBerryDeviceConfiguration(name, type, machineType, origin, id));
}

QString BlackBerryDeviceConfiguration::debugToken() const
{
    return m_debugToken;
}

void BlackBerryDeviceConfiguration::setDebugToken(const QString &debugToken)
{
    m_debugToken = debugToken;
}

void BlackBerryDeviceConfiguration::fromMap(const QVariantMap &map)
{
    RemoteLinux::LinuxDevice::fromMap(map);
    m_debugToken = map.value(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY)).toString();
}

IDevice::Ptr BlackBerryDeviceConfiguration::clone() const
{
    return Ptr(new BlackBerryDeviceConfiguration(*this));
}

bool BlackBerryDeviceConfiguration::hasDeviceTester() const
{
    // we are unable to easily verify that a device is available unless we duplicate
    // 'Connect to device' functionality, therefore disabling device-tester
    return false;
}

BlackBerryDeviceConfiguration::ConstPtr BlackBerryDeviceConfiguration::device(const Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    return dev.dynamicCast<const BlackBerryDeviceConfiguration>();
}

QString BlackBerryDeviceConfiguration::displayType() const
{
    return tr("BlackBerry");
}

IDeviceWidget *BlackBerryDeviceConfiguration::createWidget()
{
    return new BlackBerryDeviceConfigurationWidget(sharedFromThis()
                                                   .staticCast<BlackBerryDeviceConfiguration>());
}

QList<Core::Id> BlackBerryDeviceConfiguration::actionIds() const
{
    return QList<Core::Id>() << Core::Id(ConnectToDeviceActionId)
                             << Core::Id(DisconnectFromDeviceActionId)
                             << Core::Id(DeployQtLibrariesActionId);
}

QString BlackBerryDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    if (actionId == Core::Id(ConnectToDeviceActionId))
        return tr("Connect to device");
    else if (actionId == Core::Id(DisconnectFromDeviceActionId))
        return tr("Disconnect from device");
    else if (actionId == Core::Id(DeployQtLibrariesActionId))
        return tr("Deploy Qt libraries...");

    return QString();
}

void BlackBerryDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(parent);

    const BlackBerryDeviceConfiguration::ConstPtr device =
            sharedFromThis().staticCast<const BlackBerryDeviceConfiguration>();

    BlackBerryDeviceConnectionManager *connectionManager =
            BlackBerryDeviceConnectionManager::instance();
    if (actionId == Core::Id(ConnectToDeviceActionId)) {
        connectionManager->connectDevice(device);
    } else if (actionId == Core::Id(DisconnectFromDeviceActionId)
             && connectionManager->isConnected(id())) {
        connectionManager->disconnectDevice(device);
    } else if (actionId == Core::Id(DeployQtLibrariesActionId)) {
        QnxDeployQtLibrariesDialog dialog(device, QnxDeployQtLibrariesDialog::BB10, parent);
        dialog.exec();
    }
}

QVariantMap BlackBerryDeviceConfiguration::toMap() const
{
    QVariantMap map = RemoteLinux::LinuxDevice::toMap();
    map.insert(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY), m_debugToken);
    return map;
}

DeviceProcessList *BlackBerryDeviceConfiguration::createProcessListModel(QObject *parent) const
{
    return new QnxDeviceProcessList(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr BlackBerryDeviceConfiguration::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(
                new BlackBerryDeviceProcessSignalOperation(sshParameters()));
}
