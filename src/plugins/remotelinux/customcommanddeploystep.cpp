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

#include "customcommanddeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class CustomCommandDeployService : public AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::CustomCommandDeployService)

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
            emit errorMessage(tr("Remote process failed: %1").arg(m_process.errorString()));
        } else if (m_process.exitCode() != 0) {
            emit errorMessage(tr("Remote process finished with exit code %1.")
                .arg(m_process.exitCode()));
        } else {
            emit progressMessage(tr("Remote command finished successfully."));
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
        return CheckResult::failure(tr("No command line given."));

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

void CustomCommandDeployService::doDeploy()
{
    emit progressMessage(tr("Starting remote command \"%1\"...").arg(m_commandLine));
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
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::CustomCommandDeployStep)

public:
    CustomCommandDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<CustomCommandDeployService>();

        auto commandLine = addAspect<StringAspect>();
        commandLine->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
        commandLine->setLabelText(tr("Command line:"));
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
    setDisplayName(CustomCommandDeployStep::tr("Run custom remote command"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // Internal
} // RemoteLinux
