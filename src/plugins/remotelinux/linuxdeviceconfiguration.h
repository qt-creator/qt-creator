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
#ifndef LINUXDEVICECONFIGURATION_H
#define LINUXDEVICECONFIGURATION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace Utils {
class SshConnectionParameters;
class PortList;
}

namespace RemoteLinux {
namespace Internal {
class LinuxDeviceConfigurationPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT LinuxDeviceConfiguration : public ProjectExplorer::IDevice
{
public:
    typedef QSharedPointer<LinuxDeviceConfiguration> Ptr;
    typedef QSharedPointer<const LinuxDeviceConfiguration> ConstPtr;

    enum MachineType { Hardware, Emulator };

    ~LinuxDeviceConfiguration();

    Utils::PortList freePorts() const;
    Utils::SshConnectionParameters sshParameters() const;
    MachineType machineType() const;

    void setSshParameters(const Utils::SshConnectionParameters &sshParameters);
    void setFreePorts(const Utils::PortList &freePorts);

    static QString defaultPrivateKeyFilePath();
    static QString defaultPublicKeyFilePath();

    static Ptr create();
    static Ptr create(const QString &name, const QString &type, MachineType machineType,
        Origin origin = ManuallyAdded, const QString &fingerprint = QString());

    void fromMap(const QVariantMap &map);
    ProjectExplorer::IDevice::Ptr clone() const;
private:
    LinuxDeviceConfiguration();
    LinuxDeviceConfiguration(const QString &name, const QString &type, MachineType machineType,
        Origin origin, const QString &fingerprint);

    LinuxDeviceConfiguration(const LinuxDeviceConfiguration &other);
    LinuxDeviceConfiguration &operator=(const LinuxDeviceConfiguration &);

    QVariantMap toMap() const;

    Internal::LinuxDeviceConfigurationPrivate *d;
};

} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATION_H
