/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdebugsupport.h"
#include "qnxconstants.h"
#include "qnxdevice.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Qnx {
namespace Internal {

QnxDebugSupport::QnxDebugSupport(QnxRunConfiguration *runConfig, Debugger::DebuggerRunControl *runControl)
    : QnxAbstractRunSupport(runConfig, runControl)
    , m_runnable(runConfig->runnable().as<StandardRunnable>())
    , m_runControl(runControl)
    , m_pdebugPort(-1)
    , m_qmlPort(-1)
    , m_useCppDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger())
    , m_useQmlDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger())
{
    const DeviceApplicationRunner *runner = appRunner();
    connect(runner, &DeviceApplicationRunner::reportError, this, &QnxDebugSupport::handleError);
    connect(runner, &DeviceApplicationRunner::remoteProcessStarted, this, &QnxDebugSupport::handleRemoteProcessStarted);
    connect(runner, &DeviceApplicationRunner::finished, this, &QnxDebugSupport::handleRemoteProcessFinished);
    connect(runner, &DeviceApplicationRunner::reportProgress, this, &QnxDebugSupport::handleProgressReport);
    connect(runner, &DeviceApplicationRunner::remoteStdout, this, &QnxDebugSupport::handleRemoteOutput);
    connect(runner, &DeviceApplicationRunner::remoteStderr, this, &QnxDebugSupport::handleRemoteOutput);

    connect(m_runControl, &Debugger::DebuggerRunControl::requestRemoteSetup,
            this, &QnxDebugSupport::handleAdapterSetupRequested);

    const QString applicationId = Utils::FileName::fromString(runConfig->remoteExecutableFilePath()).fileName();
    IDevice::ConstPtr dev = DeviceKitInformation::device(runConfig->target()->kit());
    QnxDevice::ConstPtr qnxDevice = dev.dynamicCast<const QnxDevice>();

    m_slog2Info = new Slog2InfoRunner(applicationId, qnxDevice, this);
    connect(m_slog2Info, &Slog2InfoRunner::output, this, &QnxDebugSupport::handleApplicationOutput);
    connect(runner, &DeviceApplicationRunner::remoteProcessStarted, m_slog2Info, &Slog2InfoRunner::start);
    if (qnxDevice->qnxVersion() > 0x060500)
        connect(m_slog2Info, &Slog2InfoRunner::commandMissing, this, &QnxDebugSupport::printMissingWarning);
}

void QnxDebugSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    if (m_runControl)
        m_runControl->showMessage(tr("Preparing remote side...") + QLatin1Char('\n'), Debugger::AppStuff);
    QnxAbstractRunSupport::handleAdapterSetupRequested();
}

void QnxDebugSupport::startExecution()
{
    if (state() == Inactive)
        return;

    if (m_useCppDebugger && !setPort(m_pdebugPort))
        return;
    if (m_useQmlDebugger && !setPort(m_qmlPort))
        return;

    setState(StartingRemoteProcess);

    StandardRunnable r = m_runnable;
    QStringList arguments;
    if (m_useCppDebugger)
        arguments << QString::number(m_pdebugPort.number());
    else {
        if (m_useQmlDebugger) {
            arguments.append(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                            m_qmlPort));
        }
        arguments.append(Utils::QtcProcess::splitArgs(r.commandLineArguments));
    }

    r.executable = processExecutable();
    r.commandLineArguments = Utils::QtcProcess::joinArgs(arguments);
    r.environment = m_runnable.environment;
    r.workingDirectory = m_runnable.workingDirectory;
    appRunner()->start(device(), r);
}

void QnxDebugSupport::handleRemoteProcessStarted()
{
    QnxAbstractRunSupport::handleRemoteProcessStarted();
    if (m_runControl) {
        Debugger::RemoteSetupResult result;
        result.success = true;
        result.gdbServerPort = m_pdebugPort;
        result.qmlServerPort = m_qmlPort;
        m_runControl->notifyEngineRemoteSetupFinished(result);
    }
}

void QnxDebugSupport::handleRemoteProcessFinished(bool success)
{
    if (!m_runControl || state() == Inactive)
        return;

    if (state() == Running) {
        if (!success)
            m_runControl->notifyInferiorIll();

    } else {
        Debugger::RemoteSetupResult result;
        result.success = false;
        result.reason = tr("The %1 process closed unexpectedly.").arg(processExecutable());
        m_runControl->notifyEngineRemoteSetupFinished(result);
    }
}

void QnxDebugSupport::handleDebuggingFinished()
{
    // setFinished() will kill "pdebug", but we also have to kill
    // the inferior process, as invoking "kill" in gdb doesn't work
    // on QNX gdb
    setFinished();
    m_slog2Info->stop();
    killInferiorProcess();
}

QString QnxDebugSupport::processExecutable() const
{
    return m_useCppDebugger? QLatin1String(Constants::QNX_DEBUG_EXECUTABLE) : m_runnable.executable;
}

void QnxDebugSupport::killInferiorProcess()
{
    device()->signalOperation()->killProcess(m_runnable.executable);
}

void QnxDebugSupport::handleProgressReport(const QString &progressOutput)
{
    if (m_runControl)
        m_runControl->showMessage(progressOutput + QLatin1Char('\n'), Debugger::AppStuff);
}

void QnxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    if (m_runControl)
        m_runControl->showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void QnxDebugSupport::handleError(const QString &error)
{
    if (state() == Running) {
        if (m_runControl) {
            m_runControl->showMessage(error, Debugger::AppError);
            m_runControl->notifyInferiorIll();
        }
    } else if (state() != Inactive) {
        setFinished();
        if (m_runControl) {
            Debugger::RemoteSetupResult result;
            result.success = false;
            result.reason = tr("Initial setup failed: %1").arg(error);
            m_runControl->notifyEngineRemoteSetupFinished(result);
        }
    }
}

void QnxDebugSupport::printMissingWarning()
{
    if (m_runControl)
        m_runControl->showMessage(tr("Warning: \"slog2info\" is not found on the device, debug output not available."), Debugger::AppError);
}

void QnxDebugSupport::handleApplicationOutput(const QString &msg, Utils::OutputFormat outputFormat)
{
    Q_UNUSED(outputFormat);
    if (m_runControl)
        m_runControl->showMessage(msg, Debugger::AppOutput);
}

} // namespace Internal
} // namespace Qnx
