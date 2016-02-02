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

#include "deviceapplicationrunner.h"

#include "deviceprocess.h"
#include "../runnables.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {

namespace {
enum State { Inactive, Run };
} // anonymous namespace

class DeviceApplicationRunner::DeviceApplicationRunnerPrivate
{
public:
    DeviceProcess *deviceProcess;
    State state;
    bool stopRequested;
    bool success;
};


DeviceApplicationRunner::DeviceApplicationRunner(QObject *parent) :
    QObject(parent), d(new DeviceApplicationRunnerPrivate)
{
    d->deviceProcess = 0;
    d->state = Inactive;
}

DeviceApplicationRunner::~DeviceApplicationRunner()
{
    setFinished();
    delete d;
}

void DeviceApplicationRunner::start(const IDevice::ConstPtr &device, const Runnable &runnable)
{
    QTC_ASSERT(d->state == Inactive, return);

    d->state = Run;
    if (!device) {
        doReportError(tr("Cannot run: No device."));
        setFinished();
        return;
    }

    if (!device->canCreateProcess()) {
        doReportError(tr("Cannot run: Device is not able to create processes."));
        setFinished();
        return;
    }

    if (runnable.is<StandardRunnable>() && runnable.as<StandardRunnable>().executable.isEmpty()) {
        doReportError(tr("Cannot run: No command given."));
        setFinished();
        return;
    }

    d->stopRequested = false;
    d->success = true;

    d->deviceProcess = device->createProcess(this);
    connect(d->deviceProcess, &DeviceProcess::started,
            this, &DeviceApplicationRunner::remoteProcessStarted);
    connect(d->deviceProcess, &DeviceProcess::readyReadStandardOutput,
            this, &DeviceApplicationRunner::handleRemoteStdout);
    connect(d->deviceProcess, &DeviceProcess::readyReadStandardError,
            this, &DeviceApplicationRunner::handleRemoteStderr);
    connect(d->deviceProcess, &DeviceProcess::error,
            this, &DeviceApplicationRunner::handleApplicationError);
    connect(d->deviceProcess, &DeviceProcess::finished,
            this, &DeviceApplicationRunner::handleApplicationFinished);
    d->deviceProcess->start(runnable);
}

void DeviceApplicationRunner::stop()
{
    if (d->stopRequested)
        return;
    d->stopRequested = true;
    d->success = false;
    emit reportProgress(tr("User requested stop. Shutting down..."));
    switch (d->state) {
    case Run:
        d->deviceProcess->terminate();
        break;
    case Inactive:
        break;
    }
}

void DeviceApplicationRunner::handleApplicationError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        doReportError(tr("Application failed to start: %1")
                         .arg(d->deviceProcess->errorString()));
        setFinished();
    }
}

void DeviceApplicationRunner::setFinished()
{
    if (d->state == Inactive)
        return;

    if (d->deviceProcess) {
        d->deviceProcess->disconnect(this);
        d->deviceProcess->deleteLater();
        d->deviceProcess = 0;
    }

    d->state = Inactive;
    emit finished(d->success);
}

void DeviceApplicationRunner::handleApplicationFinished()
{
    QTC_ASSERT(d->state == Run, return);

    if (d->deviceProcess->exitStatus() == QProcess::CrashExit) {
        doReportError(d->deviceProcess->errorString());
    } else {
        const int exitCode = d->deviceProcess->exitCode();
        if (exitCode != 0) {
            doReportError(tr("Application finished with exit code %1.").arg(exitCode));
        } else {
            emit reportProgress(tr("Application finished with exit code 0."));
        }
    }
    setFinished();
}

void DeviceApplicationRunner::handleRemoteStdout()
{
    QTC_ASSERT(d->state == Run, return);
    emit remoteStdout(d->deviceProcess->readAllStandardOutput());
}

void DeviceApplicationRunner::handleRemoteStderr()
{
    QTC_ASSERT(d->state == Run, return);
    emit remoteStderr(d->deviceProcess->readAllStandardError());
}

void DeviceApplicationRunner::doReportError(const QString &message)
{
    d->success = false;
    emit reportError(message);
}

} // namespace ProjectExplorer
