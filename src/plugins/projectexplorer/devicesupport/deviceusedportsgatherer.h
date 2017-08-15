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

#include <projectexplorer/runconfiguration.h>

#include <utils/portlist.h>

namespace ProjectExplorer {
namespace Internal { class DeviceUsedPortsGathererPrivate; }
class StandardRunnable;

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGatherer : public QObject
{
    Q_OBJECT

public:
    DeviceUsedPortsGatherer(QObject *parent = 0);
    ~DeviceUsedPortsGatherer() override;

    void start(const IDevice::ConstPtr &device);
    void stop();
    Utils::Port getNextFreePort(Utils::PortList *freePorts) const; // returns -1 if no more are left
    QList<Utils::Port> usedPorts() const;

signals:
    void error(const QString &errMsg);
    void portListReady();

private:
    void handleRemoteStdOut();
    void handleRemoteStdErr();
    void handleProcessError();
    void handleProcessFinished();

    void setupUsedPorts();

    Internal::DeviceUsedPortsGathererPrivate * const d;
};

class PROJECTEXPLORER_EXPORT PortsGatherer : public RunWorker
{
    Q_OBJECT

public:
    explicit PortsGatherer(RunControl *runControl);
    ~PortsGatherer() override;

    Utils::Port findPort();

protected:
    void start() override;
    void stop() override;

private:
    DeviceUsedPortsGatherer m_portsGatherer;
    Utils::PortList m_portList;
};

} // namespace ProjectExplorer
