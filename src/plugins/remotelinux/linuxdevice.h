/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QCoreApplication>

namespace QSsh { class SshConnectionParameters; }
namespace Utils { class PortList; }

namespace RemoteLinux {
namespace Internal { class LinuxDevicePrivate; }

class REMOTELINUX_EXPORT LinuxDevice : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::LinuxDevice)

public:
    typedef QSharedPointer<LinuxDevice> Ptr;
    typedef QSharedPointer<const LinuxDevice> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);
    ProjectExplorer::IDevice::Ptr clone() const;

    bool canCreateProcess() const { return true; }
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const;
    bool canAutoDetectPorts() const;
    ProjectExplorer::PortsGatheringMethod::Ptr portsGatheringMethod() const;
    bool canCreateProcessModel() const { return true; }
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const;
    bool hasDeviceTester() const { return true; }
    ProjectExplorer::DeviceTester *createDeviceTester() const;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;
    ProjectExplorer::DeviceEnvironmentFetcher::Ptr environmentFetcher() const;

protected:
    LinuxDevice() {}
    LinuxDevice(const QString &name, Core::Id type,
                             MachineType machineType, Origin origin, Core::Id id);
    LinuxDevice(const LinuxDevice &other);

private:
    LinuxDevice &operator=(const LinuxDevice &);
};

} // namespace RemoteLinux
