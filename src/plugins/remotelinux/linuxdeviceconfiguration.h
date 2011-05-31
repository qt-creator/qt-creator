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
#ifndef LINUXDEVICECONFIGURATION_H
#define LINUXDEVICECONFIGURATION_H

#include "remotelinux_export.h"

#include <utils/ssh/sshconnection.h>

#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE


namespace RemoteLinux {
namespace Internal {
class LinuxDeviceConfigurations;
}

class REMOTELINUX_EXPORT PortList
{
public:
    void addPort(int port);
    void addRange(int startPort, int endPort);
    bool hasMore() const;
    int count() const;
    int getNext();
    QString toString() const;

private:
    typedef QPair<int, int> Range;
    QList<Range> m_ranges;
};


class REMOTELINUX_EXPORT LinuxDeviceConfiguration
{
    friend class Internal::LinuxDeviceConfigurations;
public:
    typedef QSharedPointer<const LinuxDeviceConfiguration> ConstPtr;
    typedef quint64 Id;
    enum OsVersion { Maemo5, Maemo6, Meego, GenericLinux };
    enum DeviceType { Physical, Emulator };

    PortList freePorts() const;
    Utils::SshConnectionParameters sshParameters() const { return m_sshParameters; }
    QString name() const { return m_name; }
    OsVersion osVersion() const { return m_osVersion; }
    DeviceType type() const { return m_type; }
    QString portsSpec() const { return m_portsSpec; }
    Id internalId() const { return m_internalId; }
    bool isDefault() const { return m_isDefault; }
    static QString portsRegExpr();
    static QString defaultHost(DeviceType type, OsVersion osVersion);
    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();
    static QString defaultUser(OsVersion osVersion);
    static int defaultSshPort(DeviceType type);
    static QString defaultQemuPassword(OsVersion osVersion);

    static const Id InvalidId;

private:
    typedef QSharedPointer<LinuxDeviceConfiguration> Ptr;

    LinuxDeviceConfiguration(const QString &name, OsVersion osVersion,
        DeviceType type, const Utils::SshConnectionParameters &sshParams,
        Id &nextId);
    LinuxDeviceConfiguration(const QSettings &settings, Id &nextId);
    LinuxDeviceConfiguration(const ConstPtr &other);

    LinuxDeviceConfiguration(const LinuxDeviceConfiguration &);
    LinuxDeviceConfiguration &operator=(const LinuxDeviceConfiguration &);

    static Ptr createHardwareConfig(const QString &name, OsVersion osVersion,
        const QString &hostName, const QString &privateKeyFilePath, Id &nextId);
    static Ptr createGenericLinuxConfigUsingPassword(const QString &name,
        const QString &hostName, const QString &userName,
        const QString &password, Id &nextId);
    static Ptr createGenericLinuxConfigUsingKey(const QString &name,
        const QString &hostName, const QString &userName,
        const QString &privateKeyFilePath, Id &nextId);
    static Ptr createEmulatorConfig(const QString &name, OsVersion osVersion,
        Id &nextId);
    static Ptr create(const QSettings &settings, Id &nextId);
    static Ptr create(const ConstPtr &other);

    void save(QSettings &settings) const;
    QString defaultPortsSpec(DeviceType type) const;

    Utils::SshConnectionParameters m_sshParameters;
    QString m_name;
    OsVersion m_osVersion;
    DeviceType m_type;
    QString m_portsSpec;
    bool m_isDefault;
    Id m_internalId;
};


} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATION_H
