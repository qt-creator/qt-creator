/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_QNXDEVICECONFIGURATION_H
#define QNX_INTERNAL_QNXDEVICECONFIGURATION_H

#include "qnx_export.h"

#include <remotelinux/linuxdevice.h>

namespace Qnx {

class QNX_EXPORT QnxDeviceConfiguration : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxDeviceConfiguration)

public:
    typedef QSharedPointer<QnxDeviceConfiguration> Ptr;
    typedef QSharedPointer<const QnxDeviceConfiguration> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());
    ProjectExplorer::IDevice::Ptr clone() const;

    ProjectExplorer::PortsGatheringMethod::Ptr portsGatheringMethod() const;
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;

    ProjectExplorer::DeviceTester *createDeviceTester() const;
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const;

    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);

    QString displayType() const;

    int qnxVersion() const;

    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

protected:
    QnxDeviceConfiguration();
    QnxDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType,
                           Origin origin, Core::Id id);
    QnxDeviceConfiguration(const QnxDeviceConfiguration &other);

    QString interruptProcessByNameCommandLine(const QString &filePath) const;
    QString killProcessByNameCommandLine(const QString &filePath) const;

private:
    void updateVersionNumber() const;

    mutable int m_versionNumber;
};

} // namespace Qnx

#endif // QNX_INTERNAL_QNXDEVICECONFIGURATION_H
