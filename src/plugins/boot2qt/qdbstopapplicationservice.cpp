/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbstopapplicationservice.h"

#include "qdbconstants.h"

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace Utils;

namespace Qdb {
namespace Internal {

class QdbStopApplicationServicePrivate
{
public:
    ProjectExplorer::ApplicationLauncher applicationLauncher;
    QString errorOutput;
};

QdbStopApplicationService::QdbStopApplicationService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      d(new QdbStopApplicationServicePrivate)
{

}

QdbStopApplicationService::~QdbStopApplicationService()
{
    cleanup();
    delete d;
}

void QdbStopApplicationService::handleProcessFinished()
{
    const QString failureMessage = tr("Could not check and possibly stop running application.");
    if (d->applicationLauncher.exitStatus() == QProcess::CrashExit) {
        emit errorMessage(failureMessage);
        stopDeployment();
        return;
    }

    if (d->errorOutput.contains("Could not connect: Connection refused")) {
        emit progressMessage(tr("Checked that there is no running application."));
    } else if (!d->errorOutput.isEmpty()) {
        emit stdErrData(d->errorOutput);
        emit errorMessage(failureMessage);
    } else {
        emit progressMessage(tr("Stopped the running application."));
    }

    stopDeployment();
}

void QdbStopApplicationService::handleAppendMessage(const QString &message, Utils::OutputFormat format)
{
    if (format == Utils::StdErrFormat)
        d->errorOutput.append(message);
    else
        emit stdOutData(message);
}

void QdbStopApplicationService::doDeploy()
{
    connect(&d->applicationLauncher, &ProjectExplorer::ApplicationLauncher::error,
            this, [this] { emit stdErrData(d->applicationLauncher.errorString()); });
    connect(&d->applicationLauncher, &ProjectExplorer::ApplicationLauncher::finished,
            this, &QdbStopApplicationService::handleProcessFinished);
    connect(&d->applicationLauncher, &ProjectExplorer::ApplicationLauncher::appendMessage,
            this, &QdbStopApplicationService::handleAppendMessage);

    ProjectExplorer::Runnable runnable;
    runnable.command = {Constants::AppcontrollerFilepath, {"--stop"}};
    runnable.workingDirectory = "/usr/bin";

    d->applicationLauncher.setRunnable(runnable);
    d->applicationLauncher.start(ProjectExplorer::DeviceKitAspect::device(target()->kit()));
}

void QdbStopApplicationService::stopDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void QdbStopApplicationService::cleanup()
{
    d->applicationLauncher.disconnect(this);
}

} // namespace Internal
} // namespace Qdb
