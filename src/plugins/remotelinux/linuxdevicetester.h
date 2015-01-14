/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#ifndef LINUXDEVICETESTER_H
#define LINUXDEVICETESTER_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace ProjectExplorer { class DeviceUsedPortsGatherer; }
namespace QSsh { class SshConnection; }
namespace ProjectExplorer { class DeviceUsedPortsGatherer; }

namespace RemoteLinux {

namespace Internal { class GenericLinuxDeviceTesterPrivate; }

class REMOTELINUX_EXPORT GenericLinuxDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceTester(QObject *parent = 0);
    ~GenericLinuxDeviceTester();

    void testDevice(const ProjectExplorer::IDevice::ConstPtr &deviceConfiguration);
    void stopTest();

    ProjectExplorer::DeviceUsedPortsGatherer *usedPortsGatherer() const;

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleProcessFinished(int exitStatus);
    void handlePortsGatheringError(const QString &message);
    void handlePortListReady();

private:
    void setFinished(ProjectExplorer::DeviceTester::TestResult result);

    Internal::GenericLinuxDeviceTesterPrivate * const d;
};

} // namespace RemoteLinux

#endif // LINUXDEVICETESTER_H
