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
#include "deviceconfigurationfactory.h"

#include "maemoconfigtestdialog.h"
#include "maemodeviceconfigwizard.h"
#include "maemoremoteprocessesdialog.h"

namespace RemoteLinux {
namespace Internal {
namespace {
const char * const TestDeviceActionId = "TestDeviceAction";
const char * const RemoteProcessesActionId = "RemoteProcessesAction";
} // anonymous namespace;

DeviceConfigurationFactory::DeviceConfigurationFactory(QObject *parent)
    : ILinuxDeviceConfigurationFactory(parent)
{
}

QString DeviceConfigurationFactory::displayName() const
{
    return tr("Fremantle, Harmattan, MeeGo, GenericLinux");
}

ILinuxDeviceConfigurationWizard *DeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new MaemoDeviceConfigWizard(parent);
}

bool DeviceConfigurationFactory::supportsOsType(const QString &osType) const
{
    return osType == LinuxDeviceConfiguration::Maemo5OsType
            || osType == LinuxDeviceConfiguration::HarmattanOsType
            || osType == LinuxDeviceConfiguration::MeeGoOsType
            || osType == LinuxDeviceConfiguration::GenericLinuxOsType;
}

QStringList DeviceConfigurationFactory::supportedDeviceActionIds() const
{
    return QStringList() << QLatin1String(TestDeviceActionId)
        << QLatin1String(RemoteProcessesActionId);
}

QString DeviceConfigurationFactory::displayNameForId(const QString &actionId) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));
    if (actionId == QLatin1String(TestDeviceActionId))
        return tr("Test");
    if (actionId == QLatin1String(RemoteProcessesActionId))
        return tr("Remote processes");
    return QString(); // Can't happen.
}

QDialog *DeviceConfigurationFactory::createDeviceAction(const QString &actionId,
    const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));
    if (actionId == QLatin1String(TestDeviceActionId))
        return new MaemoConfigTestDialog(deviceConfig, parent);
    if (actionId == QLatin1String(RemoteProcessesActionId))
        return new MaemoRemoteProcessesDialog(deviceConfig, parent);
    return 0; // Can't happen.
}

} // namespace Internal
} // namespace RemoteLinux
