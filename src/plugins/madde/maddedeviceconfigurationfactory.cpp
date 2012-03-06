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
#include "maddedeviceconfigurationfactory.h"

#include "maddedevicetester.h"
#include "maemoconstants.h"
#include "maemodeviceconfigwizard.h"

#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/publickeydeploymentdialog.h>
#include <remotelinux/remotelinuxprocessesdialog.h>
#include <remotelinux/remotelinuxprocesslist.h>
#include <remotelinux/remotelinux_constants.h>
#include <remotelinux/genericlinuxdeviceconfigurationwidget.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
const char MaddeDeviceTestActionId[] = "Madde.DeviceTestAction";
const char MaddeRemoteProcessesActionId[] = "Madde.RemoteProcessesAction";
} // anonymous namespace

MaddeDeviceConfigurationFactory::MaddeDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString MaddeDeviceConfigurationFactory::displayName() const
{
    return tr("Device with MADDE support (Fremantle, Harmattan, MeeGo)");
}

IDeviceWizard *MaddeDeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new MaemoDeviceConfigWizard(parent);
}

IDeviceWidget *MaddeDeviceConfigurationFactory::createWidget(const IDevice::Ptr &device,
        QWidget *parent) const
{
    return new GenericLinuxDeviceConfigurationWidget(device.staticCast<LinuxDeviceConfiguration>(),
        parent);
}

IDevice::Ptr MaddeDeviceConfigurationFactory::loadDevice(const QVariantMap &map) const
{
    QTC_ASSERT(supportsDeviceType(IDevice::typeFromMap(map)),
        return LinuxDeviceConfiguration::Ptr());
    LinuxDeviceConfiguration::Ptr device = LinuxDeviceConfiguration::create();
    device->fromMap(map);
    return device;
}

bool MaddeDeviceConfigurationFactory::supportsDeviceType(const QString &type) const
{
    return type == QLatin1String(Maemo5OsType) || type == QLatin1String(HarmattanOsType)
        || type == QLatin1String(MeeGoOsType);
}

QString MaddeDeviceConfigurationFactory::displayNameForDeviceType(const QString &deviceType) const
{
    QTC_ASSERT(supportsDeviceType(deviceType), return QString());
    if (deviceType == QLatin1String(Maemo5OsType))
        return tr("Maemo5/Fremantle");
    if (deviceType == QLatin1String(HarmattanOsType))
        return tr("MeeGo 1.2 Harmattan");
    return tr("Other MeeGo OS");
}

QStringList MaddeDeviceConfigurationFactory::supportedDeviceActionIds() const
{
    return QStringList() << QLatin1String(MaddeDeviceTestActionId)
        << QLatin1String(Constants::GenericDeployKeyToDeviceActionId)
        << QLatin1String(MaddeRemoteProcessesActionId);
}

QString MaddeDeviceConfigurationFactory::displayNameForActionId(const QString &actionId) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));

    if (actionId == QLatin1String(MaddeDeviceTestActionId))
        return tr("Test");
    if (actionId == QLatin1String(MaddeRemoteProcessesActionId))
        return tr("Remote Processes...");
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

QDialog *MaddeDeviceConfigurationFactory::createDeviceAction(const QString &actionId,
    const IDevice::ConstPtr &device, QWidget *parent) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));

    const LinuxDeviceConfiguration::ConstPtr lDevice
        = device.staticCast<const LinuxDeviceConfiguration>();
    if (actionId == QLatin1String(MaddeDeviceTestActionId))
        return new LinuxDeviceTestDialog(lDevice, new MaddeDeviceTester, parent);
    if (actionId == QLatin1String(MaddeRemoteProcessesActionId))
        return new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(lDevice), parent);
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return PublicKeyDeploymentDialog::createDialog(lDevice, parent);
    return 0; // Can't happen.
}

} // namespace Internal
} // namespace Madde
