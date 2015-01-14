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
#include "deviceapplicationrunner.h"

#include "sshdeviceprocess.h"

#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QStringList>
#include <QTimer>

using namespace QSsh;

namespace ProjectExplorer {

namespace {
enum State { Inactive, Run };
} // anonymous namespace

class DeviceApplicationRunner::DeviceApplicationRunnerPrivate
{
public:
    DeviceProcess *deviceProcess;
    Utils::Environment environment;
    QString workingDir;
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

void DeviceApplicationRunner::setEnvironment(const Utils::Environment &env)
{
    d->environment = env;
}

void DeviceApplicationRunner::setWorkingDirectory(const QString &workingDirectory)
{
    d->workingDir = workingDirectory;
}

void DeviceApplicationRunner::start(const IDevice::ConstPtr &device,
        const QString &command, const QStringList &arguments)
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

    if (command.isEmpty()) {
        doReportError(tr("Cannot run: No command given."));
        setFinished();
        return;
    }

    d->stopRequested = false;
    d->success = true;

    d->deviceProcess = device->createProcess(this);
    connect(d->deviceProcess, SIGNAL(started()), SIGNAL(remoteProcessStarted()));
    connect(d->deviceProcess, SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdout()));
    connect(d->deviceProcess, SIGNAL(readyReadStandardError()), SLOT(handleRemoteStderr()));
    connect(d->deviceProcess, SIGNAL(error(QProcess::ProcessError)),
            SLOT(handleApplicationError(QProcess::ProcessError)));
    connect(d->deviceProcess, SIGNAL(finished()), SLOT(handleApplicationFinished()));
    d->deviceProcess->setEnvironment(d->environment);
    d->deviceProcess->setWorkingDirectory(d->workingDir);
    d->deviceProcess->start(command, arguments);
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
