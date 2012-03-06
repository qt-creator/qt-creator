/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "genericlinuxdeviceconfigurationfactory.h"

#include "genericlinuxdeviceconfigurationwizard.h"
#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdeviceconfiguration.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinuxprocessesdialog.h"
#include "remotelinuxprocesslist.h"
#include "remotelinux_constants.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace RemoteLinux {

GenericLinuxDeviceConfigurationFactory::GenericLinuxDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString GenericLinuxDeviceConfigurationFactory::displayName() const
{
    return tr("Generic Linux Device");
}

IDeviceWizard *GenericLinuxDeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new GenericLinuxDeviceConfigurationWizard(parent);
}

IDeviceWidget *GenericLinuxDeviceConfigurationFactory::createWidget(const IDevice::Ptr &device,
    QWidget *parent) const
{
    return new GenericLinuxDeviceConfigurationWidget(device.staticCast<LinuxDeviceConfiguration>(),
                                                     parent);
}

IDevice::Ptr GenericLinuxDeviceConfigurationFactory::loadDevice(const QVariantMap &map) const
{
    QTC_ASSERT(supportsDeviceType(IDevice::typeFromMap(map)),
        return LinuxDeviceConfiguration::Ptr());
    LinuxDeviceConfiguration::Ptr device = LinuxDeviceConfiguration::create();
    device->fromMap(map);
    return device;
}

bool GenericLinuxDeviceConfigurationFactory::supportsDeviceType(const QString &deviceType) const
{
    return deviceType == QLatin1String(Constants::GenericLinuxOsType);
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForDeviceType(const QString &deviceType) const
{
    QTC_ASSERT(supportsDeviceType(deviceType), return QString());
    return tr("Generic Linux");
}

QStringList GenericLinuxDeviceConfigurationFactory::supportedDeviceActionIds() const
{
    return QStringList() << QLatin1String(Constants::GenericTestDeviceActionId)
        << QLatin1String(Constants::GenericDeployKeyToDeviceActionId)
        << QLatin1String(Constants::GenericRemoteProcessesActionId);
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForActionId(const QString &actionId) const
{
    QTC_ASSERT(supportedDeviceActionIds().contains(actionId), return QString());

    if (actionId == QLatin1String(Constants::GenericTestDeviceActionId))
        return tr("Test");
    if (actionId == QLatin1String(Constants::GenericRemoteProcessesActionId))
        return tr("Remote Processes...");
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

QDialog *GenericLinuxDeviceConfigurationFactory::createDeviceAction(const QString &actionId,
    const IDevice::ConstPtr &device, QWidget *parent) const
{
    QTC_ASSERT(supportedDeviceActionIds().contains(actionId), return 0);

    const LinuxDeviceConfiguration::ConstPtr lDevice
        = device.staticCast<const LinuxDeviceConfiguration>();
    if (actionId == QLatin1String(Constants::GenericTestDeviceActionId))
        return new LinuxDeviceTestDialog(lDevice, new GenericLinuxDeviceTester, parent);
    if (actionId == QLatin1String(Constants::GenericRemoteProcessesActionId))
        return new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(lDevice, parent));
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return PublicKeyDeploymentDialog::createDialog(lDevice, parent);
    return 0; // Can't happen.
}

} // namespace RemoteLinux
