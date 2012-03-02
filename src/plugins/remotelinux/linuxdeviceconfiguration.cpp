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

#include "linuxdeviceconfigurations.h"
#include "remotelinux_constants.h"

#include <utils/portlist.h>
#include <utils/ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QDesktopServices>

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

// TODO: For pre-2.6 versions. Remove in 2.8.
LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create(const QSettings &settings)
{
    return Ptr(new LinuxDeviceConfiguration(settings));
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

LinuxDeviceConfiguration::LinuxDeviceConfiguration()
    : d(new LinuxDeviceConfigurationPrivate(SshConnectionParameters(SshConnectionParameters::NoProxy)))
{
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
    d->origin = origin;
    d->attributes = attributes;
}

// TODO: For pre-2.6 versions. Remove in 2.8.
LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QSettings &settings)
    : d(new LinuxDeviceConfigurationPrivate(SshConnectionParameters::NoProxy))
{
    d->origin = ManuallyAdded;
    d->displayName = settings.value(NameKey).toString();
    d->osType = settings.value(OsTypeKey).toString();
    d->deviceType = static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt());
    d->internalId = settings.value(InternalIdKey, InvalidId).toULongLong();
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

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::create()
{
    return Ptr(new LinuxDeviceConfiguration);
}

void LinuxDeviceConfiguration::fromMap(const QVariantMap &map)
{
    d->origin = ManuallyAdded;
    d->displayName = map.value(NameKey).toString();
    d->osType = map.value(OsTypeKey).toString();
    d->deviceType = static_cast<DeviceType>(map.value(TypeKey, DefaultDeviceType).toInt());
    d->internalId = map.value(InternalIdKey, InvalidId).toULongLong();
    const QVariantMap attrMap = map.value(AttributesKey).toMap();
    for (QVariantMap::ConstIterator it = attrMap.constBegin(); it != attrMap.constEnd(); ++it)
        d->attributes.insert(it.key(), it.value());

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
    QVariantMap map;
    map.insert(NameKey, d->displayName);
    map.insert(OsTypeKey, d->osType);
    map.insert(TypeKey, d->deviceType);
    map.insert(HostKey, d->sshParameters.host);
    map.insert(SshPortKey, d->sshParameters.port);
    map.insert(PortsSpecKey, d->freePorts.toString());
    map.insert(UserNameKey, d->sshParameters.userName);
    map.insert(AuthKey, d->sshParameters.authenticationType);
    map.insert(PasswordKey, d->sshParameters.password);
    map.insert(KeyFileKey, d->sshParameters.privateKeyFile);
    map.insert(TimeoutKey, d->sshParameters.timeout);
    map.insert(InternalIdKey, d->internalId);
    QVariantMap attrMap;
    for (QVariantHash::ConstIterator it = d->attributes.constBegin();
         it != d->attributes.constEnd(); ++it) {
        attrMap.insert(it.key(), it.value());
    }
    map.insert(AttributesKey, attrMap);
    return map;
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

void LinuxDeviceConfiguration::setAttribute(const QString &name, const QVariant &value)
{
    d->attributes[name] = value;
}

bool LinuxDeviceConfiguration::isAutoDetected() const
{
    return d->origin == AutoDetected;
}

QVariantHash LinuxDeviceConfiguration::attributes() const
{
    return d->attributes;
}

QVariant LinuxDeviceConfiguration::attribute(const QString &name) const
{
    return d->attributes.value(name);
}

PortList LinuxDeviceConfiguration::freePorts() const { return d->freePorts; }
QString LinuxDeviceConfiguration::displayName() const { return d->displayName; }
QString LinuxDeviceConfiguration::osType() const { return d->osType; }

void LinuxDeviceConfiguration::setDisplayName(const QString &name) { d->displayName = name; }
void LinuxDeviceConfiguration::setInternalId(Id id) { d->internalId = id; }

const LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::InvalidId = 0;


ILinuxDeviceConfigurationWidget::ILinuxDeviceConfigurationWidget(
        const LinuxDeviceConfiguration::Ptr &deviceConfig,
        QWidget *parent)
    : QWidget(parent),
      m_deviceConfiguration(deviceConfig)
{
    QTC_CHECK(m_deviceConfiguration);
}

LinuxDeviceConfiguration::Ptr ILinuxDeviceConfigurationWidget::deviceConfiguration() const
{
    return m_deviceConfiguration;
}

} // namespace RemoteLinux
