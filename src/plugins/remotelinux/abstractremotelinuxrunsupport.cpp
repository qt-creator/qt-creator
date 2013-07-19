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

#include "abstractremotelinuxrunsupport.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>

#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxRunSupportPrivate
{
public:
    AbstractRemoteLinuxRunSupportPrivate(const RemoteLinuxRunConfiguration *runConfig)
        : state(AbstractRemoteLinuxRunSupport::Inactive),
          device(DeviceKitInformation::device(runConfig->target()->kit())),
          remoteFilePath(runConfig->remoteExecutableFilePath()),
          arguments(runConfig->arguments()),
          commandPrefix(runConfig->commandPrefix())
    {
    }

    AbstractRemoteLinuxRunSupport::State state;
    DeviceApplicationRunner appRunner;
    DeviceUsedPortsGatherer portsGatherer;
    const ProjectExplorer::IDevice::ConstPtr device;
    Utils::PortList portList;
    const QString remoteFilePath;
    const QString arguments;
    const QString commandPrefix;
};

} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxRunSupport::AbstractRemoteLinuxRunSupport(RemoteLinuxRunConfiguration *runConfig, QObject *parent)
    : QObject(parent),
      d(new AbstractRemoteLinuxRunSupportPrivate(runConfig))
{
}

AbstractRemoteLinuxRunSupport::~AbstractRemoteLinuxRunSupport()
{
    setFinished();
    delete d;
}

void AbstractRemoteLinuxRunSupport::setApplicationRunnerPreRunAction(DeviceApplicationHelperAction *action)
{
    d->appRunner.setPreRunAction(action);
}

void AbstractRemoteLinuxRunSupport::setApplicationRunnerPostRunAction(DeviceApplicationHelperAction *action)
{
    d->appRunner.setPostRunAction(action);
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
    connect(&d->portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(&d->portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));
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
    if (d->state == Running) {
        const QString stopCommand
                = d->device->processSupport()->killProcessByNameCommandLine(d->remoteFilePath);
        d->appRunner.stop(stopCommand.toUtf8());
    }
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

QString AbstractRemoteLinuxRunSupport::arguments() const
{
    return d->arguments;
}

QString AbstractRemoteLinuxRunSupport::commandPrefix() const
{
    return d->commandPrefix;
}

QString AbstractRemoteLinuxRunSupport::remoteFilePath() const
{
    return d->remoteFilePath;
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
