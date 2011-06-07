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

#include <cctype>

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

const int DefaultSshPortHW(22);
const int DefaultSshPortSim(6666);
const int DefaultGdbServerPortHW(10000);
const int DefaultGdbServerPortSim(13219);
const AuthType DefaultAuthType(Utils::SshConnectionParameters::AuthenticationByKey);
const int DefaultTimeout(30);
const LinuxDeviceConfiguration::DeviceType DefaultDeviceType(LinuxDeviceConfiguration::Physical);


class PortsSpecParser
{
    struct ParseException {
        ParseException(const char *error) : error(error) {}
        const char * const error;
    };

public:
    PortsSpecParser(const QString &portsSpec)
        : m_pos(0), m_portsSpec(portsSpec) { }

    /*
     * Grammar: Spec -> [ ElemList ]
     *          ElemList -> Elem [ ',' ElemList ]
     *          Elem -> Port [ '-' Port ]
     */
    PortList parse()
    {
        try {
            if (!atEnd())
                parseElemList();
        } catch (ParseException &e) {
            qWarning("Malformed ports specification: %s", e.error);
        }
        return m_portList;
    }

private:
    void parseElemList()
    {
        if (atEnd())
            throw ParseException("Element list empty.");
        parseElem();
        if (atEnd())
            return;
        if (nextChar() != ',') {
            throw ParseException("Element followed by something else "
                "than a comma.");
        }
        ++m_pos;
        parseElemList();
    }

    void parseElem()
    {
        const int startPort = parsePort();
        if (atEnd() || nextChar() != '-') {
            m_portList.addPort(startPort);
            return;
        }
        ++m_pos;
        const int endPort = parsePort();
        if (endPort < startPort)
            throw ParseException("Invalid range (end < start).");
        m_portList.addRange(startPort, endPort);
    }

    int parsePort()
    {
        if (atEnd())
            throw ParseException("Empty port string.");
        int port = 0;
        do {
            const char next = nextChar();
            if (!std::isdigit(next))
                break;
            port = 10*port + next - '0';
            ++m_pos;
        } while (!atEnd());
        if (port == 0 || port >= 2 << 16)
            throw ParseException("Invalid port value.");
        return port;
    }

    bool atEnd() const { return m_pos == m_portsSpec.length(); }
    char nextChar() const { return m_portsSpec.at(m_pos).toAscii(); }

    PortList m_portList;
    int m_pos;
    const QString &m_portsSpec;
};

} // anonymous namespace


void PortList::addPort(int port) { addRange(port, port); }

void PortList::addRange(int startPort, int endPort)
{
    m_ranges << Range(startPort, endPort);
}

bool PortList::hasMore() const { return !m_ranges.isEmpty(); }

int PortList::count() const
{
    int n = 0;
    foreach (const Range &r, m_ranges)
        n += r.second - r.first + 1;
    return n;
}

int PortList::getNext()
{
    Q_ASSERT(!m_ranges.isEmpty());
    Range &firstRange = m_ranges.first();
    const int next = firstRange.first++;
    if (firstRange.first > firstRange.second)
        m_ranges.removeFirst();
    return next;
}

QString PortList::toString() const
{
    QString stringRep;
    foreach (const Range &range, m_ranges) {
        stringRep += QString::number(range.first);
        if (range.second != range.first)
            stringRep += QLatin1Char('-') + QString::number(range.second);
        stringRep += QLatin1Char(',');
    }
    if (!stringRep.isEmpty())
        stringRep.remove(stringRep.length() - 1, 1); // Trailing comma.
    return stringRep;
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

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::createHardwareConfig(const QString &name,
    const QString &osType, const QString &hostName,
    const QString &privateKeyFilePath, Id &nextId)
{
    Utils::SshConnectionParameters sshParams(Utils::SshConnectionParameters::NoProxy);
    sshParams.authenticationType = Utils::SshConnectionParameters::AuthenticationByKey;
    sshParams.host = hostName;
    sshParams.userName = defaultUser(osType);
    sshParams.privateKeyFile = privateKeyFilePath;
    return Ptr(new LinuxDeviceConfiguration(name, osType, Physical, sshParams, nextId));
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::createGenericLinuxConfigUsingPassword(const QString &name,
    const QString &hostName, const QString &userName, const QString &password,
    Id &nextId)
{
    Utils::SshConnectionParameters sshParams(Utils::SshConnectionParameters::NoProxy);
    sshParams.authenticationType
        = Utils::SshConnectionParameters::AuthenticationByPassword;
    sshParams.host = hostName;
    sshParams.userName = userName;
    sshParams.password = password;
    return Ptr(new LinuxDeviceConfiguration(name, LinuxDeviceConfiguration::GenericLinuxOsType, Physical,
        sshParams, nextId));
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::createGenericLinuxConfigUsingKey(const QString &name,
    const QString &hostName, const QString &userName, const QString &privateKeyFile,
    Id &nextId)
{
    Utils::SshConnectionParameters sshParams(Utils::SshConnectionParameters::NoProxy);
    sshParams.authenticationType
        = Utils::SshConnectionParameters::AuthenticationByKey;
    sshParams.host = hostName;
    sshParams.userName = userName;
    sshParams.privateKeyFile = privateKeyFile;
    return Ptr(new LinuxDeviceConfiguration(name, LinuxDeviceConfiguration::GenericLinuxOsType, Physical,
        sshParams, nextId));
}

LinuxDeviceConfiguration::Ptr LinuxDeviceConfiguration::createEmulatorConfig(const QString &name,
    const QString &osType, Id &nextId)
{
    Utils::SshConnectionParameters sshParams(Utils::SshConnectionParameters::NoProxy);
    sshParams.authenticationType = Utils::SshConnectionParameters::AuthenticationByPassword;
    sshParams.host = defaultHost(Emulator, osType);
    sshParams.userName = defaultUser(osType);
    sshParams.password = defaultQemuPassword(osType);
    return Ptr(new LinuxDeviceConfiguration(name, osType, Emulator, sshParams, nextId));
}

LinuxDeviceConfiguration::LinuxDeviceConfiguration(const QString &name,
    const QString &osType, DeviceType devType,
    const Utils::SshConnectionParameters &sshParams, Id &nextId)
    : m_sshParameters(sshParams),
      m_name(name),
      m_osType(osType),
      m_type(devType),
      m_portsSpec(defaultPortsSpec(m_type)),
      m_isDefault(false),
      m_internalId(nextId++)
{
    m_sshParameters.port = defaultSshPort(m_type);
    m_sshParameters.timeout = DefaultTimeout;
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

    m_portsSpec = settings.value(PortsSpecKey, defaultPortsSpec(m_type)).toString();
    m_sshParameters.host = settings.value(HostKey, defaultHost(m_type, m_osType)).toString();
    m_sshParameters.port = settings.value(SshPortKey, defaultSshPort(m_type)).toInt();
    m_sshParameters.userName = settings.value(UserNameKey, defaultUser(m_osType)).toString();
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
      m_portsSpec(other->m_portsSpec),
      m_isDefault(other->m_isDefault),
      m_internalId(other->m_internalId)
{
}

QString LinuxDeviceConfiguration::portsRegExpr()
{
    const QLatin1String portExpr("(\\d)+");
    const QString listElemExpr = QString::fromLatin1("%1(-%1)?").arg(portExpr);
    return QString::fromLatin1("((%1)(,%1)*)?").arg(listElemExpr);
}

int LinuxDeviceConfiguration::defaultSshPort(DeviceType type)
{
    return type == Physical ? DefaultSshPortHW : DefaultSshPortSim;
}

QString LinuxDeviceConfiguration::defaultPortsSpec(DeviceType type) const
{
    return QLatin1String(type == Physical ? "10000-10100" : "13219,14168");
}

QString LinuxDeviceConfiguration::defaultHost(DeviceType type, const QString &osType)
{
    if (osType == Maemo5OsType || osType == HarmattanOsType || osType == MeeGoOsType)
        return QLatin1String(type == Physical ? "192.168.2.15" : "localhost");
    return QString();
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

QString LinuxDeviceConfiguration::defaultUser(const QString &osType)
{
    if (osType == Maemo5OsType || osType == HarmattanOsType)
        return QLatin1String("developer");
    if (osType == MeeGoOsType)
        return QLatin1String("meego");
        return QString();
}

QString LinuxDeviceConfiguration::defaultQemuPassword(const QString &osType)
{
    if (osType == MeeGoOsType)
        return QLatin1String("meego");
    return QString();
}

PortList LinuxDeviceConfiguration::freePorts() const
{
    return PortsSpecParser(m_portsSpec).parse();
}

void LinuxDeviceConfiguration::save(QSettings &settings) const
{
    settings.setValue(NameKey, m_name);
    settings.setValue(OsTypeKey, m_osType);
    settings.setValue(TypeKey, m_type);
    settings.setValue(HostKey, m_sshParameters.host);
    settings.setValue(SshPortKey, m_sshParameters.port);
    settings.setValue(PortsSpecKey, m_portsSpec);
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
