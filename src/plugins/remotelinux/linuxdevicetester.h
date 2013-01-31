/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

namespace Internal {
class GenericLinuxDeviceTesterPrivate;
}

class REMOTELINUX_EXPORT AbstractLinuxDeviceTester : public QObject
{
    Q_OBJECT

public:
    enum TestResult { TestSuccess, TestFailure };

    virtual void testDevice(const ProjectExplorer::IDevice::ConstPtr &deviceConfiguration) = 0;
    virtual void stopTest() = 0;

signals:
    void progressMessage(const QString &message);
    void errorMessage(const QString &message);
    void finished(RemoteLinux::AbstractLinuxDeviceTester::TestResult result);

protected:
    explicit AbstractLinuxDeviceTester(QObject *parent = 0);
};


class REMOTELINUX_EXPORT GenericLinuxDeviceTester : public AbstractLinuxDeviceTester
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
    void setFinished(TestResult result);

    Internal::GenericLinuxDeviceTesterPrivate * const d;
};

} // namespace RemoteLinux

#endif // LINUXDEVICETESTER_H
