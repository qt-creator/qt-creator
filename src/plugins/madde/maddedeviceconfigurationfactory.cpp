/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "maemoconstants.h"
#include "maemodeviceconfigwizard.h"
#include "maemoglobal.h"

#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/publickeydeploymentdialog.h>
#include <remotelinux/remotelinuxprocessesdialog.h>
#include <remotelinux/remotelinuxprocesslist.h>
#include <remotelinux/remotelinux_constants.h>
#include <utils/qtcassert.h>

using namespace RemoteLinux;

namespace Madde {
namespace Internal {

const char MaddeRemoteProcessesActionId[] = "Madde.RemoteProcessesAction";

MaddeDeviceConfigurationFactory::MaddeDeviceConfigurationFactory(QObject *parent)
    : ILinuxDeviceConfigurationFactory(parent)
{
}

QString MaddeDeviceConfigurationFactory::displayName() const
{
    return tr("Device with MADDE support (Fremantle, Harmattan, MeeGo)");
}

ILinuxDeviceConfigurationWizard *MaddeDeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new MaemoDeviceConfigWizard(parent);
}

bool MaddeDeviceConfigurationFactory::supportsOsType(const QString &osType) const
{
    return osType == QLatin1String(Maemo5OsType) || osType == QLatin1String(HarmattanOsType)
        || osType == QLatin1String(MeeGoOsType);
}

QString MaddeDeviceConfigurationFactory::displayNameForOsType(const QString &osType) const
{
    QTC_ASSERT(supportsOsType(osType), return QString());
    if (osType == QLatin1String(Maemo5OsType))
        return tr("Maemo5/Fremantle");
    if (osType == QLatin1String(HarmattanOsType))
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
    const LinuxDeviceConfiguration::ConstPtr &deviceConfig, QWidget *parent) const
{
    Q_ASSERT(supportedDeviceActionIds().contains(actionId));

    if (actionId == QLatin1String(MaddeDeviceTestActionId)) {
        QList<LinuxDeviceTester *>  tests;
        tests.append(new AuthenticationTester(deviceConfig));
        tests.append(new LinuxDeviceTester(deviceConfig,
                                           tr("Checking kernel version..."),
                                           QLatin1String("uname -rsm")));
        QString infoCmd = QLatin1String("dpkg-query -W -f "
                                        "'${Package} ${Version} ${Status}\\n' 'libqt*' | "
                                        "grep ' installed$' | cut -d' ' -f-2");
        if (deviceConfig->osType() == MeeGoOsType)
            infoCmd = QLatin1String("rpm -qa 'libqt*' --queryformat '%{NAME} %{VERSION}\\n'");
        tests.append(new LinuxDeviceTester(deviceConfig, tr("Checking for Qt libraries..."), infoCmd));
        tests.append(new LinuxDeviceTester(deviceConfig,
                                           tr("Checking for connectivity support..."),
                                           QLatin1String("test -x ") + MaemoGlobal::devrootshPath()));
        tests.append(new LinuxDeviceTester(deviceConfig,
                                           tr("Checking for QML tooling support..."),
                                           QLatin1String("test -d /usr/lib/qt4/plugins/qmltooling")));
        tests.append(new UsedPortsTester(deviceConfig));
        return new LinuxDeviceTestDialog(tests, parent);
    }
    if (actionId == QLatin1String(MaddeRemoteProcessesActionId))
        return new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(deviceConfig), parent);
    if (actionId == QLatin1String(Constants::GenericDeployKeyToDeviceActionId))
        return PublicKeyDeploymentDialog::createDialog(deviceConfig, parent);
    return 0; // Can't happen.
}

} // namespace Internal
} // namespace Madde
