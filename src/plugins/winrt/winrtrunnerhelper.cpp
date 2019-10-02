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

#include "winrtrunnerhelper.h"

#include "winrtconstants.h"
#include "winrtrunconfiguration.h"

#include <coreplugin/idocument.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace ProjectExplorer;

using namespace WinRt;
using namespace WinRt::Internal;

WinRtRunnerHelper::WinRtRunnerHelper(ProjectExplorer::RunWorker *runWorker, QString *errorMessage)
    : QObject(runWorker)
    , m_worker(runWorker)
{
    auto runControl = runWorker->runControl();

    m_device = runWorker->device().dynamicCast<const WinRtDevice>();

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(runControl->kit());
    if (!qt) {
        *errorMessage = tr("The current kit has no Qt version.");
        return;
    }

    m_runnerFilePath = qt->hostBinPath().toString() + QStringLiteral("/winrtrunner.exe");
    if (!QFile::exists(m_runnerFilePath)) {
        *errorMessage = tr("Cannot find winrtrunner.exe in \"%1\".").arg(
                    QDir::toNativeSeparators(qt->hostBinPath().toString()));
        return;
    }

    m_executableFilePath = runControl->targetFilePath().toString();

    if (m_executableFilePath.isEmpty()) {
        *errorMessage = tr("Cannot determine the executable file path for \"%1\".")
                .arg(runControl->projectFilePath().toUserOutput());
        return;
    }

    // ### we should not need to append ".exe" here.
    if (!m_executableFilePath.endsWith(QLatin1String(".exe")))
        m_executableFilePath += QStringLiteral(".exe");


    bool loopbackExemptClient = false;
    bool loopbackExemptServer = false;
    if (auto aspect = runControl->aspect<ArgumentsAspect>())
        m_arguments = aspect->arguments(runControl->macroExpander());
    if (auto aspect = runControl->aspect<UninstallAfterStopAspect>())
        m_uninstallAfterStop = aspect->value();
    if (auto aspect = runControl->aspect<LoopbackExemptClientAspect>())
        loopbackExemptClient = aspect->value();
    if (auto aspect = runControl->aspect<LoopbackExemptServerAspect>())
        loopbackExemptServer = aspect->value();
    if (loopbackExemptClient && loopbackExemptServer)
        m_loopbackArguments = QStringList{"--loopbackexempt", "clientserver"};
    else if (loopbackExemptClient)
        m_loopbackArguments = QStringList{"--loopbackexempt", "client"};
    else if (loopbackExemptServer)
        m_loopbackArguments = QStringList{"--loopbackexempt", "server"};
}

void WinRtRunnerHelper::appendMessage(const QString &message, Utils::OutputFormat format)
{
    QTC_ASSERT(m_worker, return);
    m_worker->appendMessage(message, format);
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

void WinRtRunnerHelper::onProcessReadyReadStdOut()
{
    QTC_ASSERT(m_process, return);
    appendMessage(QString::fromLocal8Bit(m_process->readAllStandardOutput()), Utils::StdOutFormat);
}

void WinRtRunnerHelper::onProcessReadyReadStdErr()
{
    QTC_ASSERT(m_process, return);
    appendMessage(QString::fromLocal8Bit(m_process->readAllStandardError()), Utils::StdErrFormat);
}

void WinRtRunnerHelper::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QTC_ASSERT(m_process, return);
    m_process->disconnect();
    m_process->deleteLater();
    m_process = nullptr;
    emit finished(exitCode, exitStatus);
}

void WinRtRunnerHelper::onProcessError(QProcess::ProcessError processError)
{
    QTC_ASSERT(m_process, return);
    appendMessage(tr("Error while executing the WinRT Runner Tool: %1\n").arg(
                      m_process->errorString()), Utils::ErrorMessageFormat);
    m_process->disconnect();
    m_process->deleteLater();
    m_process = nullptr;
    emit error(processError);
}

void WinRtRunnerHelper::startWinRtRunner(const RunConf &conf)
{
    using namespace Utils;
    CommandLine cmdLine(FilePath::fromString(m_runnerFilePath), {});
    if (m_device) {
        cmdLine.addArg("--device");
        cmdLine.addArg(QString::number(m_device->deviceId()));
    }

    QtcProcess *process = nullptr;
    bool connectProcess = false;

    switch (conf) {
    case Debug:
        cmdLine.addArg("--debug");
        cmdLine.addArg(m_debuggerExecutable);
        if (!m_debuggerArguments.isEmpty()) {
            cmdLine.addArg("--debugger-arguments");
            cmdLine.addArg(m_debuggerArguments);
        }
        Q_FALLTHROUGH();
    case Start:
        cmdLine.addArgs({"--start", "--stop", "--wait", "0"});
        connectProcess = true;
        QTC_ASSERT(!m_process, m_process->deleteLater());
        m_process = new QtcProcess(this);
        process = m_process;
        break;
    case Stop:
        cmdLine.addArg("--stop");
        process = new QtcProcess(this);
        break;
    }

    if (m_device->type() == Constants::WINRT_DEVICE_TYPE_LOCAL)
        cmdLine.addArgs({"--profile", "appx"});
    else if (m_device->type() == Constants::WINRT_DEVICE_TYPE_PHONE ||
             m_device->type() == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        cmdLine.addArgs({"--profile", "appxphone"});

    cmdLine.addArgs(m_loopbackArguments);
    cmdLine.addArg(m_executableFilePath);
    cmdLine.addArgs(m_arguments, CommandLine::Raw);

    appendMessage(cmdLine.toUserOutput(), NormalMessageFormat);

    if (connectProcess) {
        connect(process, &QProcess::started, this, &WinRtRunnerHelper::started);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &WinRtRunnerHelper::onProcessFinished);
        connect(process, &QProcess::errorOccurred, this, &WinRtRunnerHelper::onProcessError);
        connect(process, &QProcess::readyReadStandardOutput, this, &WinRtRunnerHelper::onProcessReadyReadStdOut);
        connect(process, &QProcess::readyReadStandardError, this, &WinRtRunnerHelper::onProcessReadyReadStdErr);
    }

    process->setUseCtrlCStub(true);
    process->setCommand(cmdLine);
    process->setEnvironment(m_worker->runControl()->buildEnvironment());
    process->setWorkingDirectory(QFileInfo(m_executableFilePath).absolutePath());
    process->start();
}
