/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#include "winrtrunnerhelper.h"

#include "winrtconstants.h"
#include "winrtrunconfiguration.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace WinRt;
using namespace WinRt::Internal;

WinRtRunnerHelper::WinRtRunnerHelper(WinRtRunConfiguration *runConfiguration, QString *errormessage)
    : QObject()
    , m_messenger(0)
    , m_runConfiguration(runConfiguration)
    , m_process(0)
{
    init(runConfiguration, errormessage);
}

WinRtRunnerHelper::WinRtRunnerHelper(ProjectExplorer::RunControl *runControl)
    : QObject(runControl)
    , m_messenger(runControl)
    , m_runConfiguration(0)
    , m_process(0)
{
    m_runConfiguration = qobject_cast<WinRtRunConfiguration *>(runControl->runConfiguration());
    QString errorMessage;
    if (!init(m_runConfiguration, &errorMessage))
        runControl->appendMessage(errorMessage, Utils::ErrorMessageFormat);
}

bool WinRtRunnerHelper::init(WinRtRunConfiguration *runConfiguration, QString *errorMessage)
{
    ProjectExplorer::Target *target = runConfiguration->target();
    m_device = ProjectExplorer::DeviceKitInformation::device(
                target->kit()).dynamicCast<const WinRtDevice>();

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!qt) {
        *errorMessage = tr("The current kit has no Qt version.");
        return false;
    }

    m_runnerFilePath = qt->binPath().toString() + QStringLiteral("/winrtrunner.exe");
    if (!QFile::exists(m_runnerFilePath)) {
        *errorMessage = tr("Cannot find winrtrunner.exe in \"%1\".").arg(
                    QDir::toNativeSeparators(qt->binPath().toString()));
        return false;
    }

    const QString &proFile = m_runConfiguration->proFilePath();
    m_executableFilePath = target->applicationTargets().targetForProject(proFile).toString();
    if (m_executableFilePath.isEmpty()) {
        *errorMessage = tr("Cannot determine the executable file path for \"%1\".").arg(
                    QDir::toNativeSeparators(proFile));
        return false;
    }

    // ### we should not need to append ".exe" here.
    if (!m_executableFilePath.endsWith(QLatin1String(".exe")))
        m_executableFilePath += QStringLiteral(".exe");

    m_arguments = runConfiguration->arguments();
    m_uninstallAfterStop = runConfiguration->uninstallAfterStop();

    if (ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration())
        m_environment = bc->environment();

    return true;
}

void WinRtRunnerHelper::debug(const QString &debuggerExecutable, const QString &debuggerArguments)
{
    m_debuggerExecutable = debuggerExecutable;
    m_debuggerArguments = debuggerArguments;
    startWinRtRunner(Debug);
}

void WinRtRunnerHelper::start()
{
    startWinRtRunner(Start);
}

void WinRtRunnerHelper::stop()
{
    if (m_process)
        m_process->interrupt();
    else
        startWinRtRunner(Stop);
}

bool WinRtRunnerHelper::waitForStarted(int msecs)
{
    QTC_ASSERT(m_process, return false);
    return m_process->waitForStarted(msecs);
}

void WinRtRunnerHelper::setRunControl(ProjectExplorer::RunControl *runControl)
{
    m_messenger = runControl;
}

void WinRtRunnerHelper::onProcessReadyReadStdOut()
{
    QTC_ASSERT(m_process, return);
    if (m_messenger) {
        m_messenger->appendMessage(QString::fromLocal8Bit(
                                        m_process->readAllStandardOutput()), Utils::StdOutFormat);
    }
}

void WinRtRunnerHelper::onProcessReadyReadStdErr()
{
    QTC_ASSERT(m_process, return);
    if (m_messenger) {
        m_messenger->appendMessage(QString::fromLocal8Bit(
                                        m_process->readAllStandardError()), Utils::StdErrFormat);
    }
}

void WinRtRunnerHelper::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QTC_ASSERT(m_process, return);
    m_process->disconnect();
    m_process->deleteLater();
    m_process = 0;
    emit finished(exitCode, exitStatus);
}

void WinRtRunnerHelper::onProcessError(QProcess::ProcessError processError)
{
    QTC_ASSERT(m_process, return);
    if (m_messenger) {
        m_messenger->appendMessage(tr("Error while executing the WinRT Runner Tool: %1\n").arg(
                                        m_process->errorString()),
                                    Utils::ErrorMessageFormat);
    }
    m_process->disconnect();
    m_process->deleteLater();
    m_process = 0;
    emit error(processError);
}

void WinRtRunnerHelper::startWinRtRunner(const RunConf &conf)
{
    using namespace Utils;
    QString runnerArgs;
    if (m_device) {
        QtcProcess::addArg(&runnerArgs, QStringLiteral("--device"));
        QtcProcess::addArg(&runnerArgs, QString::number(m_device->deviceId()));
    }

    Utils::QtcProcess *process = 0;
    bool connectProcess = false;

    switch (conf) {
    case Debug:
        Utils::QtcProcess::addArg(&runnerArgs, QStringLiteral("--debug"));
        Utils::QtcProcess::addArg(&runnerArgs, m_debuggerExecutable);
        if (!m_debuggerArguments.isEmpty()) {
            Utils::QtcProcess::addArg(&runnerArgs, QStringLiteral("--debugger-arguments"));
            Utils::QtcProcess::addArg(&runnerArgs, m_debuggerArguments);
        }
        // fall through
    case Start:
        QtcProcess::addArgs(&runnerArgs, QStringLiteral("--start --stop --install --wait 0"));
        connectProcess = true;
        QTC_ASSERT(!m_process, m_process->deleteLater());
        m_process = new QtcProcess(this);
        process = m_process;
        break;
    case Stop:
        Utils::QtcProcess::addArgs(&runnerArgs, QStringLiteral("--stop"));
        process = new QtcProcess(this);
        break;
    }

    QtcProcess::addArg(&runnerArgs, m_executableFilePath);
    if (!m_arguments.isEmpty())
        QtcProcess::addArgs(&runnerArgs, m_arguments);

    if (m_messenger) {
        m_messenger->appendMessage(QStringLiteral("winrtrunner ") + runnerArgs + QLatin1Char('\n'),
                                    Utils::NormalMessageFormat);
    }

    if (connectProcess) {
        connect(process, SIGNAL(started()), SIGNAL(started()));
        connect(process, SIGNAL(finished(int,QProcess::ExitStatus)),
                SLOT(onProcessFinished(int,QProcess::ExitStatus)));
        connect(process, SIGNAL(error(QProcess::ProcessError)),
                SLOT(onProcessError(QProcess::ProcessError)));
        connect(process, SIGNAL(readyReadStandardOutput()), SLOT(onProcessReadyReadStdOut()));
        connect(process, SIGNAL(readyReadStandardError()), SLOT(onProcessReadyReadStdErr()));
    }

    process->setUseCtrlCStub(true);
    process->setCommand(m_runnerFilePath, runnerArgs);
    process->setEnvironment(m_environment);
    process->setWorkingDirectory(QFileInfo(m_executableFilePath).absolutePath());
    process->start();
}
