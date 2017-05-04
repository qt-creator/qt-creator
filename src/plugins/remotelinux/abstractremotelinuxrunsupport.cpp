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
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxRunSupportPrivate
{
public:
    ApplicationLauncher launcher;
    DeviceUsedPortsGatherer portsGatherer;
    ApplicationLauncher fifoCreator;
    PortList portList;
    QString fifo;
    bool usesFifo = false;
};

} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxRunSupport::AbstractRemoteLinuxRunSupport(RunControl *runControl)
    : TargetRunner(runControl),
      d(new AbstractRemoteLinuxRunSupportPrivate)
{
}

AbstractRemoteLinuxRunSupport::~AbstractRemoteLinuxRunSupport()
{
    delete d;
}

ApplicationLauncher *AbstractRemoteLinuxRunSupport::applicationLauncher()
{
    return &d->launcher;
}

void AbstractRemoteLinuxRunSupport::setUsesFifo(bool on)
{
    d->usesFifo = on;
}

Port AbstractRemoteLinuxRunSupport::findPort() const
{
    return d->portsGatherer.getNextFreePort(&d->portList);
}

QString AbstractRemoteLinuxRunSupport::fifo() const
{
    return d->fifo;
}

void AbstractRemoteLinuxRunSupport::prepare()
{
    if (d->usesFifo)
        createRemoteFifo();
    else
        startPortsGathering();
}

void AbstractRemoteLinuxRunSupport::startPortsGathering()
{
    appendMessage(tr("Checking available ports...") + '\n', NormalMessageFormat);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::error, this, [&](const QString &msg) {
        reportFailure(msg);
    });
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::portListReady, this, [&] {
        d->portList = device()->freePorts();
        //appendMessage(tr("Found %1 free ports").arg(d->portList.count()), NormalMessageFormat);
        reportSuccess();
    });
    d->portsGatherer.start(device());
}

void AbstractRemoteLinuxRunSupport::createRemoteFifo()
{
    appendMessage(tr("Creating remote socket...") + '\n', NormalMessageFormat);

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
            reportFailure(QString("Failed to create fifo: %1").arg(QLatin1String(*errors)));
        } else {
            d->fifo = QString::fromLatin1(*output);
            //appendMessage(tr("Created fifo").arg(d->fifo), NormalMessageFormat);
            reportSuccess();
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

    d->fifoCreator.start(r, device());
}

void AbstractRemoteLinuxRunSupport::start()
{
    connect(&d->launcher, &ApplicationLauncher::remoteStderr,
            this, &AbstractRemoteLinuxRunSupport::handleRemoteErrorOutput);
    connect(&d->launcher, &ApplicationLauncher::remoteStdout,
            this, &AbstractRemoteLinuxRunSupport::handleRemoteOutput);
    connect(&d->launcher, &ApplicationLauncher::finished,
            this, &AbstractRemoteLinuxRunSupport::handleAppRunnerFinished);
    connect(&d->launcher, &ApplicationLauncher::reportProgress,
            this, &AbstractRemoteLinuxRunSupport::handleProgressReport);
    connect(&d->launcher, &ApplicationLauncher::reportError,
            this, &AbstractRemoteLinuxRunSupport::handleAppRunnerError);
    connect(&d->launcher, &ApplicationLauncher::remoteProcessStarted,
            this, &TargetRunner::reportSuccess);
    d->launcher.start(runControl()->runnable(), device());
}

void AbstractRemoteLinuxRunSupport::onFinished()
{
    d->launcher.disconnect(this);
    d->launcher.stop();
    d->portsGatherer.disconnect(this);
}

void AbstractRemoteLinuxRunSupport::handleAppRunnerFinished(bool success)
{
    success ? reportStopped() : reportFailure();
}

void AbstractRemoteLinuxRunSupport::handleAppRunnerError(const QString &error)
{
    reportFailure(error);
}

void AbstractRemoteLinuxRunSupport::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), StdOutFormat);
}

void AbstractRemoteLinuxRunSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), StdErrFormat);
}

void AbstractRemoteLinuxRunSupport::handleProgressReport(const QString &progressOutput)
{
    appendMessage(progressOutput + '\n', LogMessageFormat);
}

} // namespace RemoteLinux
