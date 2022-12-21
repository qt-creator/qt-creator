// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
using namespace Utils::Tasking;

namespace Qdb {
namespace Internal {

// QdbStopApplicationService

class QdbStopApplicationService : public RemoteLinux::AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbStopApplicationService)

private:
    bool isDeploymentNecessary() const final { return true; }
    Group deployRecipe() final;
};

Group QdbStopApplicationService::deployRecipe()
{
    const auto setupHandler = [this](QtcProcess &process) {
        const auto device = DeviceKitAspect::device(target()->kit());
        QTC_CHECK(device);
        process.setCommand({device->filePath(Constants::AppcontrollerFilepath), {"--stop"}});
        process.setWorkingDirectory("/usr/bin");
        QtcProcess *proc = &process;
        connect(proc, &QtcProcess::readyReadStandardOutput, this, [this, proc] {
            emit stdOutData(QString::fromUtf8(proc->readAllStandardOutput()));
        });
    };
    const auto doneHandler = [this](const QtcProcess &) {
        emit progressMessage(tr("Stopped the running application."));
    };
    const auto errorHandler = [this](const QtcProcess &process) {
        const QString errorOutput = process.cleanedStdErr();
        const QString failureMessage = tr("Could not check and possibly stop running application.");
        if (process.exitStatus() == QProcess::CrashExit) {
            emit errorMessage(failureMessage);
        } else if (process.result() != ProcessResult::FinishedWithSuccess) {
            emit stdErrData(process.errorString());
        } else if (errorOutput.contains("Could not connect: Connection refused")) {
            emit progressMessage(tr("Checked that there is no running application."));
        } else if (!errorOutput.isEmpty()) {
            emit stdErrData(errorOutput);
            emit errorMessage(failureMessage);
        }
    };
    const auto rootSetupHandler = [this] {
        const auto device = DeviceKitAspect::device(target()->kit());
        if (!device) {
            emit errorMessage(tr("No device to stop the application on."));
            return GroupConfig{GroupAction::StopWithError};
        }
        return GroupConfig();
    };
    const Group root {
        DynamicSetup(rootSetupHandler),
        Process(setupHandler, doneHandler, errorHandler)
    };
    return root;
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
