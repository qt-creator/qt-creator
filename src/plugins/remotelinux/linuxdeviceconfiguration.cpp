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
    QString name;
    QString osType;
    LinuxDeviceConfiguration::DeviceType deviceType;
    PortList freePorts;
    bool isDefault;
    LinuxDeviceConfiguration::Origin origin;
    LinuxDeviceConfiguration::Id internalId;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceConfiguration::~LinuxDeviceConfiguration()
{
    delete m_d;
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
    const SshConnectionParameters &sshParams, Origin origin)
{
    return Ptr(new LinuxDeviceConfiguration(name, osType, deviceType, freePorts, sshParams, origin));
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, const QString &osType,
        DeviceType deviceType, const PortList &freePorts, const SshConnectionParameters &sshParams,
        Origin origin)
    : m_d(new LinuxDeviceConfigurationPrivate(sshParams))
{
    m_d->name = name;
    m_d->osType = osType;
    m_d->deviceType = deviceType;
    m_d->freePorts = freePorts;
    m_d->isDefault = false;
    m_d->origin = origin;
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QSettings &settings, Id &nextId)
    : m_d(new LinuxDeviceConfigurationPrivate(SshConnectionParameters::NoProxy))
{
    m_d->origin = ManuallyAdded;
    m_d->name = settings.value(NameKey).toString();
    m_d->osType = settings.value(OsTypeKey).toString();
    m_d->deviceType = static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt());
    m_d->isDefault = settings.value(IsDefaultKey, false).toBool();
    m_d->internalId = settings.value(InternalIdKey, nextId).toULongLong();

    if (m_d->internalId == nextId)
        ++nextId;

    // Convert from version < 2.3.
    if (m_d->osType.isEmpty()) {
        const int oldOsType = settings.value(OldOsVersionKey, -1).toInt();
        switch (oldOsType) {
        case 0: m_d->osType = QLatin1String("Maemo5OsType"); break;
        case 1: m_d->osType = QLatin1String("HarmattanOsType"); break;
        case 2: m_d->osType = QLatin1String("MeeGoOsType"); break;
        default: m_d->osType = QLatin1String(Constants::GenericLinuxOsType);
        }
    }

    m_d->freePorts = PortList::fromString(settings.value(PortsSpecKey, QLatin1String("10000-10100")).toString());
    m_d->sshParameters.host = settings.value(HostKey).toString();
    m_d->sshParameters.port = settings.value(SshPortKey, 22).toInt();
    m_d->sshParameters.userName = settings.value(UserNameKey).toString();
    m_d->sshParameters.authenticationType
        = static_cast<AuthType>(settings.value(AuthKey, DefaultAuthType).toInt());
    m_d->sshParameters.password = settings.value(PasswordKey).toString();
    m_d->sshParameters.privateKeyFile
        = settings.value(KeyFileKey, defaultPrivateKeyFilePath()).toString();
    m_d->sshParameters.timeout = settings.value(TimeoutKey, DefaultTimeout).toInt();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration::ConstPtr &other)
    : m_d(new LinuxDeviceConfigurationPrivate(other->m_d->sshParameters))
{
    m_d->name = other->m_d->name;
    m_d->osType = other->m_d->osType;
    m_d->deviceType = other->deviceType();
    m_d->freePorts = other->freePorts();
    m_d->isDefault = other->m_d->isDefault;
    m_d->origin = other->m_d->origin;
    m_d->internalId = other->m_d->internalId;
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
    settings.setValue(NameKey, m_d->name);
    settings.setValue(OsTypeKey, m_d->osType);
    settings.setValue(TypeKey, m_d->deviceType);
    settings.setValue(HostKey, m_d->sshParameters.host);
    settings.setValue(SshPortKey, m_d->sshParameters.port);
    settings.setValue(PortsSpecKey, m_d->freePorts.toString());
    settings.setValue(UserNameKey, m_d->sshParameters.userName);
    settings.setValue(AuthKey, m_d->sshParameters.authenticationType);
    settings.setValue(PasswordKey, m_d->sshParameters.password);
    settings.setValue(KeyFileKey, m_d->sshParameters.privateKeyFile);
    settings.setValue(TimeoutKey, m_d->sshParameters.timeout);
    settings.setValue(IsDefaultKey, m_d->isDefault);
    settings.setValue(InternalIdKey, m_d->internalId);
}

SshConnectionParameters LinuxDeviceConfiguration::sshParameters() const
{
    return m_d->sshParameters;
}

LinuxDeviceConfiguration::DeviceType LinuxDeviceConfiguration::deviceType() const
{
    return m_d->deviceType;
}

LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::internalId() const
{
    return m_d->internalId;
}

void LinuxDeviceConfiguration::setSshParameters(const SshConnectionParameters &sshParameters)
{
    m_d->sshParameters = sshParameters;
}

void LinuxDeviceConfiguration::setFreePorts(const PortList &freePorts)
{
    m_d->freePorts = freePorts;
}

bool LinuxDeviceConfiguration::isAutoDetected() const
{
    return m_d->origin == AutoDetected;
}

PortList LinuxDeviceConfiguration::freePorts() const { return m_d->freePorts; }
QString LinuxDeviceConfiguration::name() const { return m_d->name; }
QString LinuxDeviceConfiguration::osType() const { return m_d->osType; }
bool LinuxDeviceConfiguration::isDefault() const { return m_d->isDefault; }

void LinuxDeviceConfiguration::setName(const QString &name) { m_d->name = name; }
void LinuxDeviceConfiguration::setInternalId(Id id) { m_d->internalId = id; }
void LinuxDeviceConfiguration::setDefault(bool isDefault) { m_d->isDefault = isDefault; }

const LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::InvalidId = 0;

} // namespace RemoteLinux
