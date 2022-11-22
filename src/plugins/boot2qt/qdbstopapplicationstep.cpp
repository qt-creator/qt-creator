// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qdbstopapplicationstep.h"

#include "qdbconstants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <remotelinux/abstractremotelinuxdeployservice.h>
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
    QdbStopApplicationService()
    {
        connect(&m_process, &QtcProcess::done,
                this, &QdbStopApplicationService::handleProcessDone);
        connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
            m_errorOutput.append(QString::fromUtf8(m_process.readAllStandardError()));
        });
        connect(&m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
            emit stdOutData(QString::fromUtf8(m_process.readAllStandardOutput()));
        });
    }

private:
    void handleProcessDone();

    bool isDeploymentNecessary() const final { return true; }

    void doDeploy() final;
    void stopDeployment() final;

    QtcProcess m_process;
    QString m_errorOutput;
};

void QdbStopApplicationService::handleProcessDone()
{
    const QString failureMessage = tr("Could not check and possibly stop running application.");

    if (m_process.exitStatus() == QProcess::CrashExit) {
        emit errorMessage(failureMessage);
    } else if (m_process.result() != ProcessResult::FinishedWithSuccess) {
        emit stdErrData(m_process.errorString());
    } else if (m_errorOutput.contains("Could not connect: Connection refused")) {
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

    m_process.setCommand({device->filePath(Constants::AppcontrollerFilepath), {"--stop"}});
    m_process.setWorkingDirectory("/usr/bin");
    m_process.start();
}

void QdbStopApplicationService::stopDeployment()
{
    m_process.close();
    handleDeploymentDone();
}

// QdbStopApplicationStep

class QdbStopApplicationStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbStopApplicationStep)

public:
    QdbStopApplicationStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new QdbStopApplicationService;
        setDeployService(service);

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
