/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "genericlinuxdeviceconfigurationfactory.h"

#include "genericlinuxdeviceconfigurationwizard.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinuxprocessesdialog.h"
#include "remotelinuxprocesslist.h"
#include "remotelinux_constants.h"

#include <utils/qtcassert.h>

namespace RemoteLinux {

GenericLinuxDeviceConfigurationFactory::GenericLinuxDeviceConfigurationFactory(QObject *parent)
    : ILinuxDeviceConfigurationFactory(parent)
{
}

QString GenericLinuxDeviceConfigurationFactory::displayName() const
{
    return tr("Generic Linux Device");
}

ILinuxDeviceConfigurationWizard *GenericLinuxDeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new GenericLinuxDeviceConfigurationWizard(parent);
}

bool GenericLinuxDeviceConfigurationFactory::supportsOsType(const QString &osType) const
{
    return osType == QLatin1String(Constants::GenericLinuxOsType);
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForOsType(const QString &osType) const
{
    QTC_ASSERT(supportsOsType(osType), return QString());
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
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));

    if (actionId == QLatin1String(Constants::GenericTestDeviceActionId))
        return tr("Test");
    if (actionId == QLatin1String(Constants::GenericRemoteProcessesActionId))
        return tr("Remote Processes");
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key");
    return QString(); // Can't happen.
}

QDialog *GenericLinuxDeviceConfigurationFactory::createDeviceAction(const QString &actionId,
    const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));

    if (actionId == QLatin1String(Constants::GenericTestDeviceActionId))
        return new LinuxDeviceTestDialog(deviceConfig, new GenericLinuxDeviceTester, parent);
    if (actionId == QLatin1String(Constants::GenericRemoteProcessesActionId)) {
        return new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(deviceConfig),
            parent);
    }
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return new PublicKeyDeploymentDialog(deviceConfig, parent);
    return 0; // Can't happen.
}

} // namespace RemoteLinux
