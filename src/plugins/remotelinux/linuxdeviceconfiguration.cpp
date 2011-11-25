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
#include "linuxdeviceconfiguration.h"

#include "portlist.h"
#include "remotelinux_constants.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QSettings>
#include <QtGui/QDesktopServices>

using namespace Utils;
typedef SshConnectionParameters::AuthenticationType AuthType;

namespace RemoteLinux {
namespace Internal {
namespace {
const QLatin1String NameKey("Name");
const QLatin1String OldOsVersionKey("OsVersion"); // Outdated, only use for upgrading.
const QLatin1String OsTypeKey("OsType");
const QLatin1String TypeKey("Type");
const QLatin1String HostKey("Host");
const QLatin1String SshPortKey("SshPort");
const QLatin1String PortsSpecKey("FreePortsSpec");
const QLatin1String UserNameKey("Uname");
const QLatin1String AuthKey("Authentication");
const QLatin1String KeyFileKey("KeyFile");
const QLatin1String PasswordKey("Password");
const QLatin1String TimeoutKey("Timeout");
const QLatin1String IsDefaultKey("IsDefault");
const QLatin1String InternalIdKey("InternalId");
const QLatin1String AttributesKey("Attributes");

const AuthType DefaultAuthType(SshConnectionParameters::AuthenticationByKey);
const int DefaultTimeout(10);
const LinuxDeviceConfiguration::DeviceType DefaultDeviceType(LinuxDeviceConfiguration::Hardware);
} // anonymous namespace

class LinuxDeviceConfigurationPrivate
{
public:
    LinuxDeviceConfigurationPrivate(const SshConnectionParameters &sshParameters)
        : sshParameters(sshParameters)
    {
    }

    SshConnectionParameters sshParameters;
    QString displayName;
    QString osType;
    LinuxDeviceConfiguration::DeviceType deviceType;
    PortList freePorts;
    bool isDefault;
    LinuxDeviceConfiguration::Origin origin;
    LinuxDeviceConfiguration::Id internalId;
    QVariantHash attributes;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceConfiguration::~LinuxDeviceConfiguration()
{
    delete d;
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QSettings &settings,
    Id &nextId)
{
    return Ptr(new LinuxDeviceConfiguration(settings, nextId));
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const ConstPtr &other)
{
    return Ptr(new LinuxDeviceConfiguration(other));
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QString &name,
    const QString &osType, DeviceType deviceType, const PortList &freePorts,
    const SshConnectionParameters &sshParams, const QVariantHash &attributes, Origin origin)
{
    return Ptr(new LinuxDeviceConfiguration(name, osType, deviceType, freePorts, sshParams,
        attributes, origin));
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, const QString &osType,
        DeviceType deviceType, const PortList &freePorts, const SshConnectionParameters &sshParams,
        const QVariantHash &attributes, Origin origin)
    : d(new LinuxDeviceConfigurationPrivate(sshParams))
{
    d->displayName = name;
    d->osType = osType;
    d->deviceType = deviceType;
    d->freePorts = freePorts;
    d->isDefault = false;
    d->origin = origin;
    d->attributes = attributes;
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QSettings &settings, Id &nextId)
    : d(new LinuxDeviceConfigurationPrivate(SshConnectionParameters::NoProxy))
{
    d->origin = ManuallyAdded;
    d->displayName = settings.value(NameKey).toString();
    d->osType = settings.value(OsTypeKey).toString();
    d->deviceType = static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt());
    d->isDefault = settings.value(IsDefaultKey, false).toBool();
    d->internalId = settings.value(InternalIdKey, nextId).toULongLong();

    if (d->internalId == nextId)
        ++nextId;

    d->attributes = settings.value(AttributesKey).toHash();

    // Convert from version < 2.3.
    if (d->osType.isEmpty()) {
        const int oldOsType = settings.value(OldOsVersionKey, -1).toInt();
        switch (oldOsType) {
        case 0: d->osType = QLatin1String("Maemo5OsType"); break;
        case 1: d->osType = QLatin1String("HarmattanOsType"); break;
        case 2: d->osType = QLatin1String("MeeGoOsType"); break;
        default: d->osType = QLatin1String(Constants::GenericLinuxOsType);
        }
    }

    d->freePorts = PortList::fromString(settings.value(PortsSpecKey, QLatin1String("10000-10100")).toString());
    d->sshParameters.host = settings.value(HostKey).toString();
    d->sshParameters.port = settings.value(SshPortKey, 22).toInt();
    d->sshParameters.userName = settings.value(UserNameKey).toString();
    d->sshParameters.authenticationType
        = static_cast<AuthType>(settings.value(AuthKey, DefaultAuthType).toInt());
    d->sshParameters.password = settings.value(PasswordKey).toString();
    d->sshParameters.privateKeyFile
        = settings.value(KeyFileKey, defaultPrivateKeyFilePath()).toString();
    d->sshParameters.timeout = settings.value(TimeoutKey, DefaultTimeout).toInt();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration::ConstPtr &other)
    : d(new LinuxDeviceConfigurationPrivate(other->d->sshParameters))
{
    d->displayName = other->d->displayName;
    d->osType = other->d->osType;
    d->deviceType = other->deviceType();
    d->freePorts = other->freePorts();
    d->isDefault = other->d->isDefault;
    d->origin = other->d->origin;
    d->internalId = other->d->internalId;
    d->attributes = other->d->attributes;
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

void LinuxDeviceConfiguration::save(QSettings &settings) const
{
    settings.setValue(NameKey, d->displayName);
    settings.setValue(OsTypeKey, d->osType);
    settings.setValue(TypeKey, d->deviceType);
    settings.setValue(HostKey, d->sshParameters.host);
    settings.setValue(SshPortKey, d->sshParameters.port);
    settings.setValue(PortsSpecKey, d->freePorts.toString());
    settings.setValue(UserNameKey, d->sshParameters.userName);
    settings.setValue(AuthKey, d->sshParameters.authenticationType);
    settings.setValue(PasswordKey, d->sshParameters.password);
    settings.setValue(KeyFileKey, d->sshParameters.privateKeyFile);
    settings.setValue(TimeoutKey, d->sshParameters.timeout);
    settings.setValue(IsDefaultKey, d->isDefault);
    settings.setValue(InternalIdKey, d->internalId);
    settings.setValue(AttributesKey, d->attributes);
}

SshConnectionParameters LinuxDeviceConfiguration::sshParameters() const
{
    return d->sshParameters;
}

LinuxDeviceConfiguration::DeviceType LinuxDeviceConfiguration::deviceType() const
{
    return d->deviceType;
}

LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::internalId() const
{
    return d->internalId;
}

void LinuxDeviceConfiguration::setSshParameters(const SshConnectionParameters &sshParameters)
{
    d->sshParameters = sshParameters;
}

void LinuxDeviceConfiguration::setFreePorts(const PortList &freePorts)
{
    d->freePorts = freePorts;
}

bool LinuxDeviceConfiguration::isAutoDetected() const
{
    return d->origin == AutoDetected;
}

QVariantHash LinuxDeviceConfiguration::attributes() const
{
    return d->attributes;
}

PortList LinuxDeviceConfiguration::freePorts() const { return d->freePorts; }
QString LinuxDeviceConfiguration::displayName() const { return d->displayName; }
QString LinuxDeviceConfiguration::osType() const { return d->osType; }
bool LinuxDeviceConfiguration::isDefault() const { return d->isDefault; }

void LinuxDeviceConfiguration::setDisplayName(const QString &name) { d->displayName = name; }
void LinuxDeviceConfiguration::setInternalId(Id id) { d->internalId = id; }
void LinuxDeviceConfiguration::setDefault(bool isDefault) { d->isDefault = isDefault; }

const LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::InvalidId = 0;

} // namespace RemoteLinux
