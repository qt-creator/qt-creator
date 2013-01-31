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

#include "remotelinuxruncontrol.h"

#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QString>
#include <QIcon>

using namespace ProjectExplorer;

namespace RemoteLinux {

class RemoteLinuxRunControl::RemoteLinuxRunControlPrivate
{
public:
    bool running;
    ProjectExplorer::DeviceApplicationRunner runner;
    IDevice::ConstPtr device;
    QString remoteExecutable;
    QString arguments;
    QString prefix;
};

RemoteLinuxRunControl::RemoteLinuxRunControl(RunConfiguration *rc)
        : RunControl(rc, ProjectExplorer::NormalRunMode), d(new RemoteLinuxRunControlPrivate)
{
    d->running = false;
    d->device = DeviceKitInformation::device(rc->target()->kit());
    const RemoteLinuxRunConfiguration * const lrc = qobject_cast<RemoteLinuxRunConfiguration *>(rc);
    d->remoteExecutable = lrc->remoteExecutableFilePath();
    d->arguments = lrc->arguments();
    d->prefix = lrc->commandPrefix();
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
    connect(&d->runner, SIGNAL(reportError(QString)), SLOT(handleErrorMessage(QString)));
    connect(&d->runner, SIGNAL(remoteStderr(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(&d->runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(&d->runner, SIGNAL(finished(bool)), SLOT(handleRunnerFinished()));
    connect(&d->runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    const QString commandLine = QString::fromLatin1("%1 %2 %3")
            .arg(d->prefix, d->remoteExecutable, d->arguments);
    d->runner.start(d->device, commandLine.toUtf8());
}

RunControl::StopResult RemoteLinuxRunControl::stop()
{
    const QString stopCommandLine
            = d->device->processSupport()->killProcessByNameCommandLine(d->remoteExecutable);
    d->runner.stop(stopCommandLine.toUtf8());
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

QIcon RemoteLinuxRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void RemoteLinuxRunControl::setApplicationRunnerPreRunAction(DeviceApplicationHelperAction *action)
{
    d->runner.setPreRunAction(action);
}

void RemoteLinuxRunControl::setApplicationRunnerPostRunAction(DeviceApplicationHelperAction *action)
{
    d->runner.setPostRunAction(action);
}

void RemoteLinuxRunControl::setFinished()
{
    d->runner.disconnect(this);
    d->running = false;
    emit finished();
}

} // namespace RemoteLinux
