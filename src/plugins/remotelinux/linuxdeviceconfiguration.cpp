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

#include <QtCore/QSettings>
#include <QtGui/QDesktopServices>

typedef Utils::SshConnectionParameters::AuthenticationType AuthType;

namespace RemoteLinux {
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

const AuthType DefaultAuthType(Utils::SshConnectionParameters::AuthenticationByKey);
const int DefaultTimeout(10);
const LinuxDeviceConfiguration::DeviceType DefaultDeviceType(LinuxDeviceConfiguration::Physical);
} // anonymous namespace


LinuxDeviceConfiguration::~LinuxDeviceConfiguration() {}

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
    const Utils::SshConnectionParameters &sshParams)
{
    return Ptr(new LinuxDeviceConfiguration(name, osType, deviceType, freePorts, sshParams));
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name, const QString &osType,
        DeviceType deviceType, const PortList &freePorts,
        const Utils::SshConnectionParameters &sshParams)
    : m_sshParameters(sshParams), m_name(name), m_osType(osType), m_type(deviceType),
      m_freePorts(freePorts), m_isDefault(false)
{
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QSettings &settings,
        Id &nextId)
    : m_sshParameters(Utils::SshConnectionParameters::NoProxy),
      m_name(settings.value(NameKey).toString()),
      m_osType(settings.value(OsTypeKey).toString()),
      m_type(static_cast<DeviceType>(settings.value(TypeKey, DefaultDeviceType).toInt())),
      m_isDefault(settings.value(IsDefaultKey, false).toBool()),
      m_internalId(settings.value(InternalIdKey, nextId).toULongLong())
{
    if (m_internalId == nextId)
        ++nextId;

    // Convert from version < 2.3.
    if (m_osType.isEmpty()) {
        const int oldOsType = settings.value(OldOsVersionKey, -1).toInt();
        switch (oldOsType) {
        case 0: m_osType = Maemo5OsType; break;
        case 1: m_osType = HarmattanOsType; break;
        case 2: m_osType = MeeGoOsType; break;
        default: m_osType = GenericLinuxOsType;
        }
    }

    m_freePorts = PortList::fromString(settings.value(PortsSpecKey, QLatin1String("10000-10100")).toString());
    m_sshParameters.host = settings.value(HostKey).toString();
    m_sshParameters.port = settings.value(SshPortKey, 22).toInt();
    m_sshParameters.userName = settings.value(UserNameKey).toString();
    m_sshParameters.authenticationType
        = static_cast<AuthType>(settings.value(AuthKey, DefaultAuthType).toInt());
    m_sshParameters.password = settings.value(PasswordKey).toString();
    m_sshParameters.privateKeyFile
        = settings.value(KeyFileKey, defaultPrivateKeyFilePath()).toString();
    m_sshParameters.timeout = settings.value(TimeoutKey, DefaultTimeout).toInt();
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const LinuxDeviceConfiguration::ConstPtr &other)
    : m_sshParameters(other->m_sshParameters),
      m_name(other->m_name),
      m_osType(other->m_osType),
      m_type(other->type()),
      m_freePorts(other->freePorts()),
      m_isDefault(other->m_isDefault),
      m_internalId(other->m_internalId)
{
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
    settings.setValue(NameKey, m_name);
    settings.setValue(OsTypeKey, m_osType);
    settings.setValue(TypeKey, m_type);
    settings.setValue(HostKey, m_sshParameters.host);
    settings.setValue(SshPortKey, m_sshParameters.port);
    settings.setValue(PortsSpecKey, m_freePorts.toString());
    settings.setValue(UserNameKey, m_sshParameters.userName);
    settings.setValue(AuthKey, m_sshParameters.authenticationType);
    settings.setValue(PasswordKey, m_sshParameters.password);
    settings.setValue(KeyFileKey, m_sshParameters.privateKeyFile);
    settings.setValue(TimeoutKey, m_sshParameters.timeout);
    settings.setValue(IsDefaultKey, m_isDefault);
    settings.setValue(InternalIdKey, m_internalId);
}

const LinuxDeviceConfiguration::Id LinuxDeviceConfiguration::InvalidId = 0;
const QString LinuxDeviceConfiguration::Maemo5OsType = QLatin1String("Maemo5OsType");
const QString LinuxDeviceConfiguration::HarmattanOsType = QLatin1String("HarmattanOsType");
const QString LinuxDeviceConfiguration::MeeGoOsType = QLatin1String("MeeGoOsType");
const QString LinuxDeviceConfiguration::GenericLinuxOsType = QLatin1String("GenericLinuxOsType");
} // namespace RemoteLinux
