/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxdebugsupport.h"
#include "qnxconstants.h"
#include "qnxdeviceconfiguration.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

using namespace Qnx;
using namespace Qnx::Internal;

QnxDebugSupport::QnxDebugSupport(QnxRunConfiguration *runConfig, Debugger::DebuggerRunControl *runControl)
    : QnxAbstractRunSupport(runConfig, runControl)
    , m_runControl(runControl)
    , m_pdebugPort(-1)
    , m_qmlPort(-1)
    , m_useCppDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger())
    , m_useQmlDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger())
{
    const DeviceApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleError(QString)));
    connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    connect(runner, SIGNAL(finished(bool)), SLOT(handleRemoteProcessFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));

    connect(m_runControl, &Debugger::DebuggerRunControl::requestRemoteSetup,
            this, &QnxDebugSupport::handleAdapterSetupRequested);

    const QString applicationId = Utils::FileName::fromString(runConfig->remoteExecutableFilePath()).fileName();
    IDevice::ConstPtr dev = DeviceKitInformation::device(runConfig->target()->kit());
    QnxDeviceConfiguration::ConstPtr qnxDevice = dev.dynamicCast<const QnxDeviceConfiguration>();

    m_slog2Info = new Slog2InfoRunner(applicationId, qnxDevice, this);
    connect(m_slog2Info, SIGNAL(output(QString,Utils::OutputFormat)), this, SLOT(handleApplicationOutput(QString,Utils::OutputFormat)));
    connect(runner, SIGNAL(remoteProcessStarted()), m_slog2Info, SLOT(start()));
    if (qnxDevice->qnxVersion() > 0x060500)
        connect(m_slog2Info, SIGNAL(commandMissing()), this, SLOT(printMissingWarning()));
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

    if (m_useQmlDebugger)
        m_runControl->startParameters().processArgs +=
                QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices, m_qmlPort);

    QStringList arguments;
    if (m_useCppDebugger)
        arguments << QString::number(m_pdebugPort);
    else if (m_useQmlDebugger && !m_useCppDebugger)
        arguments = Utils::QtcProcess::splitArgs(m_runControl->startParameters().processArgs);
    appRunner()->setEnvironment(environment());
    appRunner()->setWorkingDirectory(workingDirectory());
    appRunner()->start(device(), executable(), arguments);
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
    if (m_runControl || state() == Inactive)
        return;

    if (state() == Running) {
        if (!success)
            m_runControl->notifyInferiorIll();

    } else {
        Debugger::RemoteSetupResult result;
        result.success = false;
        result.reason = tr("The %1 process closed unexpectedly.").arg(executable());
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

QString QnxDebugSupport::executable() const
{
    return m_useCppDebugger? QLatin1String(Constants::QNX_DEBUG_EXECUTABLE) : QnxAbstractRunSupport::executable();
}

void QnxDebugSupport::killInferiorProcess()
{
    device()->signalOperation()->killProcess(QnxAbstractRunSupport::executable());
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
