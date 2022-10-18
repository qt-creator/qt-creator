// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class CustomCommandDeployService : public AbstractRemoteLinuxDeployService
{
public:
    CustomCommandDeployService();

    void setCommandLine(const QString &commandLine);

    bool isDeploymentNecessary() const override { return true; }
    CheckResult isDeploymentPossible() const override;

protected:
    void doDeploy() override;
    void stopDeployment() override;

    QString m_commandLine;
    QtcProcess m_process;
};

CustomCommandDeployService::CustomCommandDeployService()
{
    connect(&m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdOutData(QString::fromUtf8(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
        emit stdErrData(QString::fromUtf8(m_process.readAllStandardError()));
    });
    connect(&m_process, &QtcProcess::done, this, [this] {
        if (m_process.error() != QProcess::UnknownError
                || m_process.exitStatus() != QProcess::NormalExit) {
            emit errorMessage(Tr::tr("Remote process failed: %1").arg(m_process.errorString()));
        } else if (m_process.exitCode() != 0) {
            emit errorMessage(Tr::tr("Remote process finished with exit code %1.")
                .arg(m_process.exitCode()));
        } else {
            emit progressMessage(Tr::tr("Remote command finished successfully."));
        }
        stopDeployment();
    });
}

void CustomCommandDeployService::setCommandLine(const QString &commandLine)
{
    m_commandLine = commandLine;
}

CheckResult CustomCommandDeployService::isDeploymentPossible() const
{
    if (m_commandLine.isEmpty())
        return CheckResult::failure(Tr::tr("No command line given."));

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

void CustomCommandDeployService::doDeploy()
{
    emit progressMessage(Tr::tr("Starting remote command \"%1\"...").arg(m_commandLine));
    m_process.setCommand({deviceConfiguration()->filePath("/bin/sh"),
                             {"-c", m_commandLine}});
    m_process.start();
}

void CustomCommandDeployService::stopDeployment()
{
    m_process.close();
    handleDeploymentDone();
}

class CustomCommandDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    CustomCommandDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new CustomCommandDeployService;
        setDeployService(service);

        auto commandLine = addAspect<StringAspect>();
        commandLine->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
        commandLine->setLabelText(Tr::tr("Command line:"));
        commandLine->setDisplayStyle(StringAspect::LineEditDisplay);
        commandLine->setHistoryCompleter("RemoteLinuxCustomCommandDeploymentStep.History");

        setInternalInitializer([service, commandLine] {
            service->setCommandLine(commandLine->value().trimmed());
            return service->isDeploymentPossible();
        });

        addMacroExpander();
    }
};


// CustomCommandDeployStepFactory

CustomCommandDeployStepFactory::CustomCommandDeployStepFactory()
{
    registerStep<CustomCommandDeployStep>(Constants::CustomCommandDeployStepId);
    setDisplayName(Tr::tr("Run custom remote command"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
