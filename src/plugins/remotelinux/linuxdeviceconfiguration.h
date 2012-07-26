/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#ifndef LINUXDEVICECONFIGURATION_H
#define LINUXDEVICECONFIGURATION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QCoreApplication>

namespace QSsh { class SshConnectionParameters; }
namespace Utils { class PortList; }

namespace RemoteLinux {
namespace Internal {
class LinuxDeviceConfigurationPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT LinuxDeviceConfiguration : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(LinuxDeviceConfiguration)
public:
    typedef QSharedPointer<LinuxDeviceConfiguration> Ptr;
    typedef QSharedPointer<const LinuxDeviceConfiguration> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent) const;
    ProjectExplorer::IDevice::Ptr clone() const;

protected:
    LinuxDeviceConfiguration() {}
    LinuxDeviceConfiguration(const QString &name, Core::Id type,
                             MachineType machineType, Origin origin, Core::Id id);
    LinuxDeviceConfiguration(const LinuxDeviceConfiguration &other);

private:
    LinuxDeviceConfiguration &operator=(const LinuxDeviceConfiguration &);
};

} // namespace RemoteLinux

#endif // LINUXDEVICECONFIGURATION_H
