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
#include "linuxdeviceconfiguration.h"

#include "genericlinuxdeviceconfigurationwidget.h"
#include "linuxdevicetestdialog.h"
#include "publickeydeploymentdialog.h"
#include "remotelinuxprocessesdialog.h"
#include "remotelinuxprocesslist.h"
#include "remotelinux_constants.h"

#include <utils/portlist.h>
#include <utils/ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QStringList>

using namespace Utils;
typedef SshConnectionParameters::AuthenticationType AuthType;

namespace RemoteLinux {
namespace Internal {
namespace {
const QLatin1String MachineTypeKey("Type");
const QLatin1String HostKey("Host");
const QLatin1String SshPortKey("SshPort");
const QLatin1String PortsSpecKey("FreePortsSpec");
const QLatin1String UserNameKey("Uname");
const QLatin1String AuthKey("Authentication");
const QLatin1String KeyFileKey("KeyFile");
const QLatin1String PasswordKey("Password");
const QLatin1String TimeoutKey("Timeout");

const AuthType DefaultAuthType(SshConnectionParameters::AuthenticationByKey);
const int DefaultTimeout(10);
const LinuxDeviceConfiguration::MachineType DefaultMachineType(LinuxDeviceConfiguration::Hardware);
} // anonymous namespace

class LinuxDeviceConfigurationPrivate
{
public:
    SshConnectionParameters sshParameters;
    LinuxDeviceConfiguration::MachineType machineType;
    PortList freePorts;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceConfiguration::~LinuxDeviceConfiguration()
{
    delete d;
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QString &name,
       Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new LinuxDeviceConfiguration(name, type, machineType, origin, id));
}

QString LinuxDeviceConfiguration::displayType() const
{
    return tr("Generic Linux");
}

ProjectExplorer::IDeviceWidget *LinuxDeviceConfiguration::createWidget()
{
    return new GenericLinuxDeviceConfigurationWidget(sharedFromThis()
            .staticCast<LinuxDeviceConfiguration>());
}

QList<Core::Id> LinuxDeviceConfiguration::actionIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericTestDeviceActionId)
        << Core::Id(Constants::GenericDeployKeyToDeviceActionId)
        << Core::Id(Constants::GenericRemoteProcessesActionId);
}

QString LinuxDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        return tr("Test");
    if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        return tr("Remote Processes...");
    if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        return tr("Deploy Public Key...");
    return QString(); // Can't happen.
}

void LinuxDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    QDialog *d;
    const LinuxDeviceConfiguration::ConstPtr device
            = sharedFromThis().staticCast<const LinuxDeviceConfiguration>();
    if (actionId == Core::Id(Constants::GenericTestDeviceActionId))
        d = new LinuxDeviceTestDialog(device, new GenericLinuxDeviceTester, parent);
    else if (actionId == Core::Id(Constants::GenericRemoteProcessesActionId))
        d = new RemoteLinuxProcessesDialog(new GenericRemoteLinuxProcessList(device, parent));
    else if (actionId == Core::Id(Constants::GenericDeployKeyToDeviceActionId))
        d = PublicKeyDeploymentDialog::createDialog(device, parent);
    if (d)
        d->exec();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration() : d(new LinuxDeviceConfigurationPrivate)
{
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, Core::Id type,
        MachineType machineType, Origin origin, Core::Id id)
    : IDevice(type, origin, id), d(new LinuxDeviceConfigurationPrivate)
{
    setDisplayName(name);
    d->machineType = machineType;
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration &other)
    : IDevice(other), d(new LinuxDeviceConfigurationPrivate)
{
    d->machineType = other.machineType();
    d->freePorts = other.freePorts();
    d->sshParameters = other.d->sshParameters;
}

QString LinuxDeviceConfiguration::defaultPrivateKeyFilePath()
{
    return QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/id_rsa");
}

QString LinuxDeviceConfiguration::defaultPublicKeyFilePath()
{
    return defaultPrivateKeyFilePath() + QLatin1String(".pub");
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create()
{
    return Ptr(new LinuxDeviceConfiguration);
}

void LinuxDeviceConfiguration::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    d->machineType = static_cast<MachineType>(map.value(MachineTypeKey, DefaultMachineType).toInt());
    d->freePorts = PortList::fromString(map.value(PortsSpecKey,
        QLatin1String("10000-10100")).toString());
    d->sshParameters.host = map.value(HostKey).toString();
    d->sshParameters.port = map.value(SshPortKey, 22).toInt();
    d->sshParameters.userName = map.value(UserNameKey).toString();
    d->sshParameters.authenticationType
        = static_cast<AuthType>(map.value(AuthKey, DefaultAuthType).toInt());
    d->sshParameters.password = map.value(PasswordKey).toString();
    d->sshParameters.privateKeyFile = map.value(KeyFileKey, defaultPrivateKeyFilePath()).toString();
    d->sshParameters.timeout = map.value(TimeoutKey, DefaultTimeout).toInt();
}

QVariantMap LinuxDeviceConfiguration::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(MachineTypeKey, d->machineType);
    map.insert(HostKey, d->sshParameters.host);
    map.insert(SshPortKey, d->sshParameters.port);
    map.insert(PortsSpecKey, d->freePorts.toString());
    map.insert(UserNameKey, d->sshParameters.userName);
    map.insert(AuthKey, d->sshParameters.authenticationType);
    map.insert(PasswordKey, d->sshParameters.password);
    map.insert(KeyFileKey, d->sshParameters.privateKeyFile);
    map.insert(TimeoutKey, d->sshParameters.timeout);
    return map;
}

ProjectExplorer::IDevice::Ptr LinuxDeviceConfiguration::clone() const
{
    return Ptr(new LinuxDeviceConfiguration(*this));
}

SshConnectionParameters LinuxDeviceConfiguration::sshParameters() const
{
    return d->sshParameters;
}

LinuxDeviceConfiguration::MachineType LinuxDeviceConfiguration::machineType() const

{
    return d->machineType;
}

void LinuxDeviceConfiguration::setSshParameters(const SshConnectionParameters &sshParameters)
{
    d->sshParameters = sshParameters;
}

void LinuxDeviceConfiguration::setFreePorts(const PortList &freePorts)
{
    d->freePorts = freePorts;
}

PortList LinuxDeviceConfiguration::freePorts() const { return d->freePorts; }

} // namespace RemoteLinux
