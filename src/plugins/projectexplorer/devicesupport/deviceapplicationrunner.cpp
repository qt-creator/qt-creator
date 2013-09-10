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
enum State { Inactive, Connecting, PreRun, Run, PostRun };
} // anonymous namespace

class DeviceApplicationRunner::DeviceApplicationRunnerPrivate
{
public:
    SshConnection *connection;
    DeviceApplicationHelperAction *preRunAction;
    DeviceApplicationHelperAction *postRunAction;
    DeviceProcess *deviceProcess;
    IDevice::ConstPtr device;
    QTimer stopTimer;
    QString command;
    QStringList arguments;
    Utils::Environment environment;
    QString workingDir;
    State state;
    bool stopRequested;
    bool success;
};


DeviceApplicationHelperAction::DeviceApplicationHelperAction(QObject *parent) : QObject(parent)
{
}

DeviceApplicationHelperAction::~DeviceApplicationHelperAction()
{
}


DeviceApplicationRunner::DeviceApplicationRunner(QObject *parent) :
    QObject(parent), d(new DeviceApplicationRunnerPrivate)
{
    d->preRunAction = 0;
    d->postRunAction = 0;
    d->connection = 0;
    d->deviceProcess = 0;
    d->state = Inactive;

    d->stopTimer.setSingleShot(true);
    connect(&d->stopTimer, SIGNAL(timeout()), SLOT(handleStopTimeout()));
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
    QTC_ASSERT(device->canCreateProcess(), return);
    QTC_ASSERT(d->state == Inactive, return);

    d->device = device;
    d->command = command;
    d->arguments = arguments;
    d->stopRequested = false;
    d->success = true;

    connectToServer();
}

void DeviceApplicationRunner::stop()
{
    if (d->stopRequested)
        return;
    d->stopRequested = true;
    d->success = false;
    emit reportProgress(tr("User requested stop. Shutting down..."));
    switch (d->state) {
    case Connecting:
        setFinished();
        break;
    case PreRun:
        d->preRunAction->stop();
        break;
    case Run:
        d->stopTimer.start(10000);
        d->deviceProcess->terminate();
        break;
    case PostRun:
        d->postRunAction->stop();
        break;
    case Inactive:
        break;
    }
}

void DeviceApplicationRunner::setPreRunAction(DeviceApplicationHelperAction *action)
{
    addAction(d->preRunAction, action);
}

void DeviceApplicationRunner::setPostRunAction(DeviceApplicationHelperAction *action)
{
    addAction(d->postRunAction, action);
}

void DeviceApplicationRunner::connectToServer()
{
    QTC_CHECK(!d->connection);

    d->state = Connecting;

    if (!d->device) {
        emit reportError(tr("Cannot run: No device."));
        setFinished();
        return;
    }

    d->connection = QSsh::acquireConnection(d->device->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionFailure()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        connect(d->connection, SIGNAL(connected()), SLOT(handleConnected()));
        if (d->connection->state() == QSsh::SshConnection::Unconnected)
            d->connection->connectToHost();
    }
}

void DeviceApplicationRunner::executePreRunAction()
{
    QTC_ASSERT(d->state == Connecting, return);

    d->state = PreRun;
    if (d->preRunAction)
        d->preRunAction->start();
    else
        runApplication();
}

void DeviceApplicationRunner::executePostRunAction()
{
    QTC_ASSERT(d->state == PreRun || d->state == Run, return);

    d->state = PostRun;
    if (d->postRunAction)
        d->postRunAction->start();
    else
        setFinished();
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
    if (d->connection) {
        d->connection->disconnect(this);
        QSsh::releaseConnection(d->connection);
        d->connection = 0;
    }

    d->state = Inactive;
    emit finished(d->success);
}

void DeviceApplicationRunner::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    if (d->stopRequested) {
        setFinished();
        return;
    }

    executePreRunAction();
}

void DeviceApplicationRunner::handleConnectionFailure()
{
    QTC_ASSERT(d->state != Inactive, return);

    emit reportError(tr("SSH connection failed: %1").arg(d->connection->errorString()));
    d->success = false;
    switch (d->state) {
    case Inactive:
        break; // Can't happen.
    case Connecting:
        setFinished();
        break;
    case PreRun:
        d->preRunAction->stop();
        break;
    case Run:
        d->stopTimer.stop();
        d->deviceProcess->disconnect(this);
        executePostRunAction();
        break;
    case PostRun:
        d->postRunAction->stop();
        break;
    }
}

void DeviceApplicationRunner::handleHelperActionFinished(bool success)
{
    switch (d->state) {
    case Inactive:
        break;
    case PreRun:
        if (success && d->success) {
            runApplication();
        } else if (success && !d->success) {
            executePostRunAction();
        } else {
            d->success = false;
            setFinished();
        }
        break;
    case PostRun:
        if (!success)
            d->success = false;
        setFinished();
        break;
    default:
        QTC_CHECK(false);
    }
}

void DeviceApplicationRunner::addAction(DeviceApplicationHelperAction *&target,
        DeviceApplicationHelperAction *source)
{
    QTC_ASSERT(d->state == Inactive, return);

    if (target)
        disconnect(target, 0, this, 0);
    target = source;
    if (target) {
        connect(target, SIGNAL(finished(bool)), SLOT(handleHelperActionFinished(bool)));
        connect(target, SIGNAL(reportProgress(QString)), SIGNAL(reportProgress(QString)));
        connect(target, SIGNAL(reportError(QString)), SIGNAL(reportError(QString)));
    }
}

void DeviceApplicationRunner::handleStopTimeout()
{
    QTC_ASSERT(d->stopRequested && d->state == Run, return);

    emit reportError(tr("Application did not finish in time, aborting."));
    d->success = false;
    setFinished();
}

void DeviceApplicationRunner::handleApplicationFinished()
{
    QTC_ASSERT(d->state == Run, return);

    d->stopTimer.stop();
    if (d->deviceProcess->exitStatus() == QProcess::CrashExit) {
        emit reportError(tr("Remote application crashed: %1").arg(d->deviceProcess->errorString()));
        d->success = false;
    } else {
        const int exitCode = d->deviceProcess->exitCode();
        if (exitCode != 0) {
            emit reportError(tr("Remote application finished with exit code %1.").arg(exitCode));
            d->success = false;
        } else {
            emit reportProgress(tr("Remote application finished with exit code 0."));
        }
    }
    executePostRunAction();
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

void DeviceApplicationRunner::runApplication()
{
    QTC_ASSERT(d->state == PreRun, return);

    d->state = Run;
    d->deviceProcess = d->device->createProcess(this);
    connect(d->deviceProcess, SIGNAL(started()), SIGNAL(remoteProcessStarted()));
    connect(d->deviceProcess, SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdout()));
    connect(d->deviceProcess, SIGNAL(readyReadStandardError()), SLOT(handleRemoteStderr()));
    connect(d->deviceProcess, SIGNAL(finished()), SLOT(handleApplicationFinished()));
    d->deviceProcess->setEnvironment(d->environment);
    d->deviceProcess->setWorkingDirectory(d->workingDir);
    d->deviceProcess->start(d->command, d->arguments);
}

} // namespace ProjectExplorer
