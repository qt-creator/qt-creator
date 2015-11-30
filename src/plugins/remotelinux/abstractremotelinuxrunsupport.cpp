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

#include "abstractremotelinuxrunsupport.h"
#include "abstractremotelinuxrunconfiguration.h"

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <utils/environment.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxRunSupportPrivate
{
public:
    AbstractRemoteLinuxRunSupportPrivate(const AbstractRemoteLinuxRunConfiguration *runConfig)
        : state(AbstractRemoteLinuxRunSupport::Inactive),
          device(DeviceKitInformation::device(runConfig->target()->kit())),
          remoteFilePath(runConfig->remoteExecutableFilePath()),
          arguments(runConfig->arguments()),
          environment(runConfig->environment()),
          workingDir(runConfig->workingDirectory())
    {
    }

    AbstractRemoteLinuxRunSupport::State state;
    DeviceApplicationRunner appRunner;
    DeviceUsedPortsGatherer portsGatherer;
    const IDevice::ConstPtr device;
    Utils::PortList portList;
    const QString remoteFilePath;
    const QStringList arguments;
    const Utils::Environment environment;
    const QString workingDir;
};

} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxRunSupport::AbstractRemoteLinuxRunSupport(AbstractRemoteLinuxRunConfiguration *runConfig, QObject *parent)
    : QObject(parent),
      d(new AbstractRemoteLinuxRunSupportPrivate(runConfig))
{
}

AbstractRemoteLinuxRunSupport::~AbstractRemoteLinuxRunSupport()
{
    setFinished();
    delete d;
}

void AbstractRemoteLinuxRunSupport::setState(AbstractRemoteLinuxRunSupport::State state)
{
    d->state = state;
}

AbstractRemoteLinuxRunSupport::State AbstractRemoteLinuxRunSupport::state() const
{
    return d->state;
}

void AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(d->state == Inactive, return);
    d->state = GatheringPorts;
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &AbstractRemoteLinuxRunSupport::handlePortsGathererError);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &AbstractRemoteLinuxRunSupport::handlePortListReady);
    d->portsGatherer.start(d->device);
}

void AbstractRemoteLinuxRunSupport::handlePortsGathererError(const QString &message)
{
    QTC_ASSERT(d->state == GatheringPorts, return);
    handleAdapterSetupFailed(message);
}

void AbstractRemoteLinuxRunSupport::handlePortListReady()
{
    QTC_ASSERT(d->state == GatheringPorts, return);

    d->portList = d->device->freePorts();
    startExecution();
}

void AbstractRemoteLinuxRunSupport::handleAdapterSetupFailed(const QString &)
{
    setFinished();
    reset();
}

void AbstractRemoteLinuxRunSupport::handleAdapterSetupDone()
{
    d->state = Running;
}

void AbstractRemoteLinuxRunSupport::setFinished()
{
    if (d->state == Inactive)
        return;
    if (d->state == Running)
        d->appRunner.stop();
    d->state = Inactive;
}

bool AbstractRemoteLinuxRunSupport::setPort(int &port)
{
    port = d->portsGatherer.getNextFreePort(&d->portList);
    if (port == -1) {
        handleAdapterSetupFailed(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}

QStringList AbstractRemoteLinuxRunSupport::arguments() const
{
    return d->arguments;
}

QString AbstractRemoteLinuxRunSupport::remoteFilePath() const
{
    return d->remoteFilePath;
}

Utils::Environment AbstractRemoteLinuxRunSupport::environment() const
{
    return d->environment;
}

QString AbstractRemoteLinuxRunSupport::workingDirectory() const
{
    return d->workingDir;
}

const IDevice::ConstPtr AbstractRemoteLinuxRunSupport::device() const
{
    return d->device;
}

void AbstractRemoteLinuxRunSupport::reset()
{
    d->portsGatherer.disconnect(this);
    d->appRunner.disconnect(this);
    d->state = Inactive;
}

DeviceApplicationRunner *AbstractRemoteLinuxRunSupport::appRunner() const
{
    return &d->appRunner;
}

} // namespace RemoteLinux
