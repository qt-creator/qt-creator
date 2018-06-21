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
#include <projectexplorer/runconfiguration.h>
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
    auto runConfiguration = runWorker->runControl()->runConfiguration();

    ProjectExplorer::Target *target = runConfiguration->target();
    m_device = runWorker->device().dynamicCast<const WinRtDevice>();

    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!qt) {
        *errorMessage = tr("The current kit has no Qt version.");
        return;
    }

    m_runnerFilePath = qt->binPath().toString() + QStringLiteral("/winrtrunner.exe");
    if (!QFile::exists(m_runnerFilePath)) {
        *errorMessage = tr("Cannot find winrtrunner.exe in \"%1\".").arg(
                    QDir::toNativeSeparators(qt->binPath().toString()));
        return;
    }

    const BuildTargetInfo bti = target->applicationTargets().buildTargetInfo(runConfiguration->buildKey());
    m_executableFilePath = bti.targetFilePath.toString();

    if (m_executableFilePath.isEmpty()) {
        *errorMessage = tr("Cannot determine the executable file path for \"%1\".").arg(
                    QDir::toNativeSeparators(bti.projectFilePath.toString()));
        return;
    }

    // ### we should not need to append ".exe" here.
    if (!m_executableFilePath.endsWith(QLatin1String(".exe")))
        m_executableFilePath += QStringLiteral(".exe");

    if (auto aspect = runConfiguration->extraAspect<ArgumentsAspect>())
        m_arguments = aspect->arguments();
    if (auto aspect = runConfiguration->extraAspect<UninstallAfterStopAspect>())
        m_uninstallAfterStop = aspect->value();

    if (ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration())
        m_environment = bc->environment();
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
    m_process = 0;
    emit finished(exitCode, exitStatus);
}

void WinRtRunnerHelper::onProcessError(QProcess::ProcessError processError)
{
    QTC_ASSERT(m_process, return);
    appendMessage(tr("Error while executing the WinRT Runner Tool: %1\n").arg(
                      m_process->errorString()), Utils::ErrorMessageFormat);
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

    QtcProcess *process = 0;
    bool connectProcess = false;

    switch (conf) {
    case Debug:
        QtcProcess::addArg(&runnerArgs, QStringLiteral("--debug"));
        QtcProcess::addArg(&runnerArgs, m_debuggerExecutable);
        if (!m_debuggerArguments.isEmpty()) {
            QtcProcess::addArg(&runnerArgs, QStringLiteral("--debugger-arguments"));
            QtcProcess::addArg(&runnerArgs, m_debuggerArguments);
        }
        Q_FALLTHROUGH();
    case Start:
        QtcProcess::addArgs(&runnerArgs, QStringLiteral("--start --stop --install --wait 0"));
        connectProcess = true;
        QTC_ASSERT(!m_process, m_process->deleteLater());
        m_process = new QtcProcess(this);
        process = m_process;
        break;
    case Stop:
        QtcProcess::addArgs(&runnerArgs, QStringLiteral("--stop"));
        process = new QtcProcess(this);
        break;
    }

    if (m_device->type() == Constants::WINRT_DEVICE_TYPE_LOCAL)
        QtcProcess::addArgs(&runnerArgs, QStringLiteral("--profile appx"));
    else if (m_device->type() == Constants::WINRT_DEVICE_TYPE_PHONE ||
             m_device->type() == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        QtcProcess::addArgs(&runnerArgs, QStringLiteral("--profile appxphone"));

    QtcProcess::addArg(&runnerArgs, m_executableFilePath);
    if (!m_arguments.isEmpty())
        QtcProcess::addArgs(&runnerArgs, m_arguments);

    appendMessage(QStringLiteral("winrtrunner ") + runnerArgs + QLatin1Char('\n'), NormalMessageFormat);

    if (connectProcess) {
        connect(process, &QProcess::started, this, &WinRtRunnerHelper::started);
        connect(process,
                static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                this, &WinRtRunnerHelper::onProcessFinished);
        connect(process, &QProcess::errorOccurred, this, &WinRtRunnerHelper::onProcessError);
        connect(process, &QProcess::readyReadStandardOutput, this, &WinRtRunnerHelper::onProcessReadyReadStdOut);
        connect(process, &QProcess::readyReadStandardError, this, &WinRtRunnerHelper::onProcessReadyReadStdErr);
    }

    process->setUseCtrlCStub(true);
    process->setCommand(m_runnerFilePath, runnerArgs);
    process->setEnvironment(m_environment);
    process->setWorkingDirectory(QFileInfo(m_executableFilePath).absolutePath());
    process->start();
}
