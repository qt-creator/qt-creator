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

#include "remotelinuxruncontrol.h"

#include "abstractremotelinuxrunconfiguration.h"

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>

#include <QString>
#include <QIcon>

using namespace ProjectExplorer;

namespace RemoteLinux {

class RemoteLinuxRunControl::RemoteLinuxRunControlPrivate
{
public:
    bool running;
    DeviceApplicationRunner runner;
    IDevice::ConstPtr device;
    QString remoteExecutable;
    QStringList arguments;
    Utils::Environment environment;
    QString workingDir;
};

RemoteLinuxRunControl::RemoteLinuxRunControl(RunConfiguration *rc)
        : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE), d(new RemoteLinuxRunControlPrivate)
{
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));

    d->running = false;
    d->device = DeviceKitInformation::device(rc->target()->kit());
    const AbstractRemoteLinuxRunConfiguration * const lrc = qobject_cast<AbstractRemoteLinuxRunConfiguration *>(rc);
    d->remoteExecutable = lrc->remoteExecutableFilePath();
    d->arguments = lrc->arguments();
    d->environment = lrc->environment();
    d->workingDir = lrc->workingDirectory();
}

RemoteLinuxRunControl::~RemoteLinuxRunControl()
{
    delete d;
}

void RemoteLinuxRunControl::start()
{
    d->running = true;
    emit started();
    d->runner.disconnect(this);
    connect(&d->runner, &DeviceApplicationRunner::reportError,
            this, &RemoteLinuxRunControl::handleErrorMessage);
    connect(&d->runner, &DeviceApplicationRunner::remoteStderr,
            this, &RemoteLinuxRunControl::handleRemoteErrorOutput);
    connect(&d->runner, &DeviceApplicationRunner::remoteStdout,
            this, &RemoteLinuxRunControl::handleRemoteOutput);
    connect(&d->runner, &DeviceApplicationRunner::finished,
            this, &RemoteLinuxRunControl::handleRunnerFinished);
    connect(&d->runner, &DeviceApplicationRunner::reportProgress,
            this, &RemoteLinuxRunControl::handleProgressReport);
    d->runner.setEnvironment(d->environment);
    d->runner.setWorkingDirectory(d->workingDir);
    d->runner.start(d->device, d->remoteExecutable, d->arguments);
}

RunControl::StopResult RemoteLinuxRunControl::stop()
{
    d->runner.stop();
    return AsynchronousStop;
}

void RemoteLinuxRunControl::handleErrorMessage(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
}

void RemoteLinuxRunControl::handleRunnerFinished()
{
    setFinished();
}

void RemoteLinuxRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void RemoteLinuxRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

void RemoteLinuxRunControl::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

bool RemoteLinuxRunControl::isRunning() const
{
    return d->running;
}

void RemoteLinuxRunControl::setFinished()
{
    d->runner.disconnect(this);
    d->running = false;
    emit finished();
}

} // namespace RemoteLinux
