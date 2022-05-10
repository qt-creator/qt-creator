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

#include "qdbstopapplicationstep.h"

#include "qdbconstants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

// QdbStopApplicationService

class QdbStopApplicationService : public RemoteLinux::AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbStopApplicationService)

public:
    QdbStopApplicationService() {}
    ~QdbStopApplicationService() { cleanup(); }

private:
    void handleProcessDone();

    bool isDeploymentNecessary() const final { return true; }

    void doDeploy() final;
    void stopDeployment() final;

    void cleanup();

    QtcProcess m_process;
    QString m_errorOutput;
};

void QdbStopApplicationService::handleProcessDone()
{
    const QString failureMessage = tr("Could not check and possibly stop running application.");

    if (m_process.exitStatus() == QProcess::CrashExit) {
        emit errorMessage(failureMessage);
        stopDeployment();
        return;
    }

    if (m_process.result() != ProcessResult::FinishedWithSuccess) {
        emit stdErrData(m_process.errorString());
        return;
    }

    if (m_errorOutput.contains("Could not connect: Connection refused")) {
        emit progressMessage(tr("Checked that there is no running application."));
    } else if (!m_errorOutput.isEmpty()) {
        emit stdErrData(m_errorOutput);
        emit errorMessage(failureMessage);
    } else {
        emit progressMessage(tr("Stopped the running application."));
    }

    stopDeployment();
}

void QdbStopApplicationService::doDeploy()
{
    auto device = DeviceKitAspect::device(target()->kit());
    QTC_ASSERT(device, return);

    connect(&m_process, &QtcProcess::done,
            this, &QdbStopApplicationService::handleProcessDone);

    connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
        m_errorOutput.append(QString::fromUtf8(m_process.readAllStandardError()));
    });
    connect(&m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdOutData(QString::fromUtf8(m_process.readAllStandardOutput()));
    });

    m_process.setCommand({device->filePath(Constants::AppcontrollerFilepath), {"--stop"}});
    m_process.setWorkingDirectory("/usr/bin");
    m_process.start();
}

void QdbStopApplicationService::stopDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void QdbStopApplicationService::cleanup()
{
    m_process.close();
}


// QdbStopApplicationStep

class QdbStopApplicationStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbStopApplicationStep)

public:
    QdbStopApplicationStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<QdbStopApplicationService>();

        setWidgetExpandedByDefault(false);

        setInternalInitializer([service] { return service->isDeploymentPossible(); });
    }
};


// QdbStopApplicationStepFactory

QdbStopApplicationStepFactory::QdbStopApplicationStepFactory()
{
    registerStep<QdbStopApplicationStep>(Constants::QdbStopApplicationStepId);
    setDisplayName(QdbStopApplicationStep::tr("Stop already running application"));
    setSupportedDeviceType(Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace Qdb
