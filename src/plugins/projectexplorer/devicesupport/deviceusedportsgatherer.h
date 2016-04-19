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

#include "idevice.h"

namespace Utils {
class Port;
class PortList;
} // namespace Utils

namespace ProjectExplorer {
namespace Internal { class DeviceUsedPortsGathererPrivate; }

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGatherer : public QObject
{
    Q_OBJECT

public:
    DeviceUsedPortsGatherer(QObject *parent = 0);
    ~DeviceUsedPortsGatherer() override;

    void start(const ProjectExplorer::IDevice::ConstPtr &device);
    void stop();
    Utils::Port getNextFreePort(Utils::PortList *freePorts) const; // returns -1 if no more are left
    QList<Utils::Port> usedPorts() const;

signals:
    void error(const QString &errMsg);
    void portListReady();

private:
    void handleConnectionEstablished();
    void handleConnectionError();
    void handleProcessClosed(int exitStatus);
    void handleRemoteStdOut();
    void handleRemoteStdErr();

    void setupUsedPorts();

    Internal::DeviceUsedPortsGathererPrivate * const d;
};

} // namespace ProjectExplorer
