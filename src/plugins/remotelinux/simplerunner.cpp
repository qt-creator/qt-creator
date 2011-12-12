/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "simplerunner.h"

#include "linuxdeviceconfiguration.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshremoteprocessrunner.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshconnectionmanager.h>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class SimpleRunnerPrivate
{
public:
    SimpleRunnerPrivate() : process(0), sshParameters(Utils::SshConnectionParameters::NoProxy)
    { }

    QString commandline;

    SshRemoteProcessRunner *process;
    Utils::SshConnectionParameters sshParameters;
};

} // namespace Internal

using namespace Internal;

SimpleRunner::SimpleRunner(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration,
                           const QString &commandline) :
    d(new SimpleRunnerPrivate)
{
    QTC_ASSERT(!deviceConfiguration.isNull(), return);
    d->commandline = commandline;
    d->sshParameters = deviceConfiguration->sshParameters();
}

SimpleRunner::~SimpleRunner()
{
    delete d;
}

QString SimpleRunner::commandLine() const
{
    return d->commandline;
}

void SimpleRunner::run()
{
    QTC_ASSERT(!d->process, return);

    d->process = new SshRemoteProcessRunner(this);

    connect(d->process, SIGNAL(connectionError()), this, SLOT(handleConnectionFailure()));
    connect(d->process, SIGNAL(processClosed(int)), this, SLOT(handleProcessFinished(int)));
    connect(d->process, SIGNAL(processOutputAvailable(QByteArray)), this, SLOT(handleStdOutput(QByteArray)));
    connect(d->process, SIGNAL(processErrorOutputAvailable(QByteArray)),
            this, SLOT(handleStdError(QByteArray)));

    emit aboutToStart();
    d->process->run(d->commandline.toUtf8(), sshParameters());
    emit started();
}

void SimpleRunner::cancel()
{
    if (!d->process)
        return;

    d->process->cancel();
    emit finished(-1);
}

void SimpleRunner::handleConnectionFailure()
{
    QTC_ASSERT(d->process, return);

    emit errorMessage(tr("SSH connection failure: %1\n").arg(d->process->lastConnectionErrorString()));
    delete d->process;
    d->process = 0;

    emit finished(processFinished(SshRemoteProcess::FailedToStart));
}

void SimpleRunner::handleProcessFinished(int exitStatus)
{
    emit finished(processFinished(exitStatus));
}

void SimpleRunner::handleStdOutput(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    emit progressMessage(QString::fromUtf8(data));
}

void SimpleRunner::handleStdError(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    emit errorMessage(QString::fromUtf8(data));
}

int SimpleRunner::processFinished(int exitStatus)
{
    int exitCode = -1;
    if (d->process)
        exitCode = d->process->processExitCode();
    delete d->process;
    d->process = 0;

    return (exitStatus == SshRemoteProcess::ExitedNormally && exitCode == 0) ? 0 : 1;
}

void SimpleRunner::setCommandLine(const QString &cmd)
{
    QTC_ASSERT(d->commandline.isEmpty(), return);
    d->commandline = cmd;
}

SshConnectionParameters SimpleRunner::sshParameters() const
{
    return d->sshParameters;
}

} // namespace RemoteLinux
