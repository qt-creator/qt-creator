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

#include "abstractremotelinuxrunsupport.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxRunSupportPrivate
{
public:
    AbstractRemoteLinuxRunSupportPrivate(const RunConfiguration *runConfig)
        : state(AbstractRemoteLinuxRunSupport::Inactive),
          runnable(runConfig->runnable().as<StandardRunnable>()),
          device(DeviceKitInformation::device(runConfig->target()->kit()))
    {
    }

    AbstractRemoteLinuxRunSupport::State state;
    StandardRunnable runnable;
    ApplicationLauncher appLauncher;
    DeviceUsedPortsGatherer portsGatherer;
    ApplicationLauncher fifoCreator;
    const IDevice::ConstPtr device;
    Utils::PortList portList;
    QString fifo;
};

} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxRunSupport::AbstractRemoteLinuxRunSupport(RunConfiguration *runConfig, QObject *parent)
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

void AbstractRemoteLinuxRunSupport::handleResourcesError(const QString &message)
{
    QTC_ASSERT(d->state == GatheringResources, return);
    handleAdapterSetupFailed(message);
}

void AbstractRemoteLinuxRunSupport::handleResourcesAvailable()
{
    QTC_ASSERT(d->state == GatheringResources, return);

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
        d->appLauncher.stop();
    d->state = Inactive;
}

Utils::Port AbstractRemoteLinuxRunSupport::findPort() const
{
    return d->portsGatherer.getNextFreePort(&d->portList);
}

QString AbstractRemoteLinuxRunSupport::fifo() const
{
    return d->fifo;
}

void AbstractRemoteLinuxRunSupport::startPortsGathering()
{
    QTC_ASSERT(d->state == Inactive, return);
    d->state = GatheringResources;
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &AbstractRemoteLinuxRunSupport::handleResourcesError);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &AbstractRemoteLinuxRunSupport::handleResourcesAvailable);
    d->portsGatherer.start(d->device);
}

void AbstractRemoteLinuxRunSupport::createRemoteFifo()
{
    QTC_ASSERT(d->state == Inactive, return);
    d->state = GatheringResources;

    StandardRunnable r;
    r.executable = QLatin1String("/bin/sh");
    r.commandLineArguments = "-c 'd=`mktemp -d` && mkfifo $d/fifo && echo -n $d/fifo'";
    r.workingDirectory = QLatin1String("/tmp");
    r.runMode = ApplicationLauncher::Console;

    QSharedPointer<QByteArray> output(new QByteArray);
    QSharedPointer<QByteArray> errors(new QByteArray);

    connect(&d->fifoCreator, &ApplicationLauncher::finished,
            this, [this, output, errors](bool success) {
        if (!success) {
            handleResourcesError(QString("Failed to create fifo: %1").arg(QLatin1String(*errors)));
        } else {
            d->fifo = QString::fromLatin1(*output);
            handleResourcesAvailable();
        }
    });

    connect(&d->fifoCreator, &ApplicationLauncher::remoteStdout,
            this, [output](const QByteArray &data) {
        output->append(data);
    });

    connect(&d->fifoCreator, &ApplicationLauncher::remoteStderr,
            this, [errors](const QByteArray &data) {
        errors->append(data);
    });

    d->fifoCreator.start(r, d->device);
}

const IDevice::ConstPtr AbstractRemoteLinuxRunSupport::device() const
{
    return d->device;
}

const StandardRunnable &AbstractRemoteLinuxRunSupport::runnable() const
{
    return d->runnable;
}

void AbstractRemoteLinuxRunSupport::reset()
{
    d->portsGatherer.disconnect(this);
    d->appLauncher.disconnect(this);
    d->state = Inactive;
}

ApplicationLauncher *AbstractRemoteLinuxRunSupport::appRunner() const
{
    return &d->appLauncher;
}

} // namespace RemoteLinux
