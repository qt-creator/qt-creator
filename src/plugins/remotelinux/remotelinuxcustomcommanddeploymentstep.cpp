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

#include "remotelinuxcustomcommanddeploymentstep.h"

#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

// RemoteLinuxCustomCommandDeployService

class RemoteLinuxCustomCommandDeployService : public AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::RemoteLinuxCustomCommandDeployService)

public:
    RemoteLinuxCustomCommandDeployService();

    void setCommandLine(const QString &commandLine);

    bool isDeploymentNecessary() const override { return true; }
    CheckResult isDeploymentPossible() const override;

protected:
    void doDeploy() override;
    void stopDeployment() override;

    QString m_commandLine;
    QtcProcess m_process;
};

RemoteLinuxCustomCommandDeployService::RemoteLinuxCustomCommandDeployService()
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

void RemoteLinuxCustomCommandDeployService::setCommandLine(const QString &commandLine)
{
    m_commandLine = commandLine;
}

CheckResult RemoteLinuxCustomCommandDeployService::isDeploymentPossible() const
{
    if (m_commandLine.isEmpty())
        return CheckResult::failure(tr("No command line given."));

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

void RemoteLinuxCustomCommandDeployService::doDeploy()
{
    emit progressMessage(tr("Starting remote command \"%1\"...").arg(m_commandLine));
    m_process.setCommand({deviceConfiguration()->filePath("/bin/sh"),
                             {"-c", m_commandLine}});
    m_process.start();
}

void RemoteLinuxCustomCommandDeployService::stopDeployment()
{
    m_process.close();
    handleDeploymentDone();
}

} // Internal


// RemoteLinuxCustomCommandDeploymentStep

RemoteLinuxCustomCommandDeploymentStep::RemoteLinuxCustomCommandDeploymentStep
        (BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<Internal::RemoteLinuxCustomCommandDeployService>();

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

RemoteLinuxCustomCommandDeploymentStep::~RemoteLinuxCustomCommandDeploymentStep() = default;

Utils::Id RemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return Constants::CustomCommandDeployStepId;
}

QString RemoteLinuxCustomCommandDeploymentStep::displayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux
