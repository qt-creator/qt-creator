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
#include "maemoconfigtestdialog.h"
#include "maemoremoteprocessesdialog.h"
#include "publickeydeploymentdialog.h"

#include <utils/qtcassert.h>

namespace RemoteLinux {
namespace Internal {
namespace {
const char * const TestDeviceActionId = "TestDeviceAction";
const char * const DeployKeyToDeviceActionId = "DeployKeyToDeviceAction";
const char * const RemoteProcessesActionId = "RemoteProcessesAction";
} // anonymous namespace;

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
    return osType == LinuxDeviceConfiguration::GenericLinuxOsType;
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForOsType(const QString &osType) const
{
    QTC_ASSERT(supportsOsType(osType), return QString());
    return tr("Generic Linux");
}

QStringList GenericLinuxDeviceConfigurationFactory::supportedDeviceActionIds() const
{
    return QStringList() << QLatin1String(TestDeviceActionId)
        << QLatin1String(DeployKeyToDeviceActionId) << QLatin1String(RemoteProcessesActionId);
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForActionId(const QString &actionId) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));
    if (actionId == QLatin1String(TestDeviceActionId))
        return tr("Test");
    if (actionId == QLatin1String(RemoteProcessesActionId))
        return tr("Remote Processes");
    if (actionId == QLatin1String(DeployKeyToDeviceActionId))
        return tr("Deploy Public Key");
    return QString(); // Can't happen.
}

QDialog *GenericLinuxDeviceConfigurationFactory::createDeviceAction(const QString &actionId,
    const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));
    if (actionId == QLatin1String(TestDeviceActionId))
        return new MaemoConfigTestDialog(deviceConfig, parent);
    if (actionId == QLatin1String(RemoteProcessesActionId))
        return new MaemoRemoteProcessesDialog(deviceConfig, parent);
    if (actionId == QLatin1String(DeployKeyToDeviceActionId))
        return new PublicKeyDeploymentDialog(deviceConfig, parent);
    return 0; // Can't happen.
}

} // namespace Internal
} // namespace RemoteLinux
