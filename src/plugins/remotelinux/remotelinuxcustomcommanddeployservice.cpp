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

#include "remotelinuxcustomcommanddeployservice.h"

#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, Running };
}

class RemoteLinuxCustomCommandDeployservicePrivate
{
public:
    RemoteLinuxCustomCommandDeployservicePrivate() : state(Inactive), runner(0) { }

    QString commandLine;
    State state;
    SshRemoteProcessRunner *runner;
};

} // namespace Internal

using namespace Internal;


RemoteLinuxCustomCommandDeployService::RemoteLinuxCustomCommandDeployService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent), d(new RemoteLinuxCustomCommandDeployservicePrivate)
{
}

RemoteLinuxCustomCommandDeployService::~RemoteLinuxCustomCommandDeployService()
{
    delete d;
}

void RemoteLinuxCustomCommandDeployService::setCommandLine(const QString &commandLine)
{
    QTC_ASSERT(d->state == Inactive, return);

    d->commandLine = commandLine;
}

bool RemoteLinuxCustomCommandDeployService::isDeploymentPossible(QString *whyNot) const
{
    QTC_ASSERT(d->state == Inactive, return false);

    if (!AbstractRemoteLinuxDeployService::isDeploymentPossible(whyNot))
        return false;
    if (d->commandLine.isEmpty()) {
        if (whyNot)
            *whyNot = tr("No command line given.");
        return false;
    }

    return true;
}

void RemoteLinuxCustomCommandDeployService::doDeploy()
{
    QTC_ASSERT(d->state == Inactive, handleDeploymentDone());

    if (!d->runner)
        d->runner = new SshRemoteProcessRunner(this);
    connect(d->runner, SIGNAL(readyReadStandardOutput()), SLOT(handleStdout()));
    connect(d->runner, SIGNAL(readyReadStandardError()), SLOT(handleStderr()));
    connect(d->runner, SIGNAL(processClosed(int)), SLOT(handleProcessClosed(int)));

    emit progressMessage(tr("Starting remote command '%1'...").arg(d->commandLine));
    d->state = Running;
    d->runner->run(d->commandLine.toUtf8(), deviceConfiguration()->sshParameters());
}

void RemoteLinuxCustomCommandDeployService::stopDeployment()
{
    QTC_ASSERT(d->state == Running, return);

    disconnect(d->runner, 0, this, 0);
    d->runner->cancel();
    d->state = Inactive;
    handleDeploymentDone();
}

void RemoteLinuxCustomCommandDeployService::handleStdout()
{
    emit stdOutData(QString::fromUtf8(d->runner->readAllStandardOutput()));
}

void RemoteLinuxCustomCommandDeployService::handleStderr()
{
    emit stdErrData(QString::fromUtf8(d->runner->readAllStandardError()));
}

void RemoteLinuxCustomCommandDeployService::handleProcessClosed(int exitStatus)
{
    QTC_ASSERT(d->state == Running, return);

    if (exitStatus == SshRemoteProcess::FailedToStart) {
        emit errorMessage(tr("Remote process failed to start."));
    } else if (exitStatus == SshRemoteProcess::CrashExit) {
        emit errorMessage(tr("Remote process was killed by a signal."));
    } else if (d->runner->processExitCode() != 0) {
        emit errorMessage(tr("Remote process finished with exit code %1.")
            .arg(d->runner->processExitCode()));
    } else {
        emit progressMessage(tr("Remote command finished successfully."));
    }

    stopDeployment();
}

} // namespace RemoteLinux
