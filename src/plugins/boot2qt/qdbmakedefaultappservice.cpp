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

#include "qdbmakedefaultappservice.h"

#include "qdbconstants.h"
#include "qdbrunconfiguration.h"

#include <ssh/sshremoteprocessrunner.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Qdb {
namespace Internal {

class QdbMakeDefaultAppServicePrivate
{
public:
    bool makeDefault;
    QSsh::SshRemoteProcessRunner *processRunner;
};

QdbMakeDefaultAppService::QdbMakeDefaultAppService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      d(new QdbMakeDefaultAppServicePrivate)
{
    d->processRunner = 0;
    d->makeDefault = true;
}

QdbMakeDefaultAppService::~QdbMakeDefaultAppService()
{
    cleanup();
    delete d;
}

void QdbMakeDefaultAppService::setMakeDefault(bool makeDefault)
{
    d->makeDefault = makeDefault;
}

void QdbMakeDefaultAppService::handleStdErr()
{
    emit stdErrData(QString::fromUtf8(d->processRunner->readAllStandardError()));
}

void QdbMakeDefaultAppService::handleProcessFinished(const QString &error)
{
    if (!error.isEmpty()) {
        emit errorMessage(tr("Remote process failed: %1").arg(error));
        stopDeployment();
        return;
    }

    QByteArray processOutput = d->processRunner->readAllStandardOutput();

    if (d->makeDefault)
        emit progressMessage(tr("Application set as the default one."));
    else
        emit progressMessage(tr("Reset the default application."));

    stopDeployment();
}

void QdbMakeDefaultAppService::doDeploy()
{
    d->processRunner = new QSsh::SshRemoteProcessRunner;
    connect(d->processRunner, &QSsh::SshRemoteProcessRunner::processClosed,
            this, &QdbMakeDefaultAppService::handleProcessFinished);
    connect(d->processRunner, &QSsh::SshRemoteProcessRunner::readyReadStandardError,
            this, &QdbMakeDefaultAppService::handleStdErr);

    QString remoteExe;

    if (ProjectExplorer::RunConfiguration *rc = target()->activeRunConfiguration()) {
        if (auto exeAspect = rc->aspect<ProjectExplorer::ExecutableAspect>())
            remoteExe = exeAspect->executable().toString();
    }

    QString command = Constants::AppcontrollerFilepath;
    command += d->makeDefault && !remoteExe.isEmpty() ? QStringLiteral(" --make-default ") + remoteExe
                                                      : QStringLiteral(" --remove-default");

    d->processRunner->run(command, deviceConfiguration()->sshParameters());
}

void QdbMakeDefaultAppService::stopDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void QdbMakeDefaultAppService::cleanup()
{
    if (d->processRunner) {
        disconnect(d->processRunner, 0, this, 0);
        d->processRunner->cancel();
        delete d->processRunner;
        d->processRunner = 0;
    }
}

} // namespace Internal
} // namespace Qdb
