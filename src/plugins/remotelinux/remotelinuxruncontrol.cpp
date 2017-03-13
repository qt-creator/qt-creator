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

#include "remotelinuxruncontrol.h"

#include <projectexplorer/applicationlauncher.h>

#include <utils/utilsicons.h>

using namespace ProjectExplorer;

namespace RemoteLinux {

class RemoteLinuxRunControl::RemoteLinuxRunControlPrivate
{
public:
    ApplicationLauncher launcher;
};

RemoteLinuxRunControl::RemoteLinuxRunControl(RunConfiguration *rc)
        : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE), d(new RemoteLinuxRunControlPrivate)
{
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
    setRunnable(rc->runnable());
}

RemoteLinuxRunControl::~RemoteLinuxRunControl()
{
    delete d;
}

void RemoteLinuxRunControl::start()
{
    reportApplicationStart();
    d->launcher.disconnect(this);
    connect(&d->launcher, &ApplicationLauncher::reportError,
            this, &RemoteLinuxRunControl::handleErrorMessage);
    connect(&d->launcher, &ApplicationLauncher::remoteStderr,
            this, &RemoteLinuxRunControl::handleRemoteErrorOutput);
    connect(&d->launcher, &ApplicationLauncher::remoteStdout,
            this, &RemoteLinuxRunControl::handleRemoteOutput);
    connect(&d->launcher, &ApplicationLauncher::finished,
            this, &RemoteLinuxRunControl::handleRunnerFinished);
    connect(&d->launcher, &ApplicationLauncher::reportProgress,
            this, &RemoteLinuxRunControl::handleProgressReport);
    d->launcher.start(runnable(), device());
}

RunControl::StopResult RemoteLinuxRunControl::stop()
{
    d->launcher.stop();
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

void RemoteLinuxRunControl::setFinished()
{
    d->launcher.disconnect(this);
    reportApplicationStop();
}

} // namespace RemoteLinux
