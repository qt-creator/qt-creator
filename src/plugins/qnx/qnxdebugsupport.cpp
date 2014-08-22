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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxdebugsupport.h"
#include "qnxconstants.h"
#include "qnxdeviceconfiguration.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace RemoteLinux;

using namespace Qnx;
using namespace Qnx::Internal;

QnxDebugSupport::QnxDebugSupport(QnxRunConfiguration *runConfig, Debugger::DebuggerEngine *engine)
    : QnxAbstractRunSupport(runConfig, engine)
    , m_engine(engine)
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

    connect(m_engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleAdapterSetupRequested()));

    const QString applicationId = QFileInfo(runConfig->remoteExecutableFilePath()).fileName();
    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(runConfig->target()->kit());
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

    if (m_engine)
        m_engine->showMessage(tr("Preparing remote side...") + QLatin1Char('\n'), Debugger::AppStuff);
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
        m_engine->startParameters().processArgs += QString::fromLatin1(" -qmljsdebugger=port:%1,block").arg(m_qmlPort);

    QStringList arguments;
    if (m_useCppDebugger)
        arguments << QString::number(m_pdebugPort);
    else if (m_useQmlDebugger && !m_useCppDebugger)
        arguments = Utils::QtcProcess::splitArgs(m_engine->startParameters().processArgs);
    appRunner()->setEnvironment(environment());
    appRunner()->setWorkingDirectory(workingDirectory());
    appRunner()->start(device(), executable(), arguments);
}

void QnxDebugSupport::handleRemoteProcessStarted()
{
    QnxAbstractRunSupport::handleRemoteProcessStarted();
    if (m_engine)
        m_engine->notifyEngineRemoteSetupDone(m_pdebugPort, m_qmlPort);
}

void QnxDebugSupport::handleRemoteProcessFinished(bool success)
{
    if (m_engine || state() == Inactive)
        return;

    if (state() == Running) {
        if (!success)
            m_engine->notifyInferiorIll();

    } else {
        const QString errorMsg = tr("The %1 process closed unexpectedly.").arg(executable());
        m_engine->notifyEngineRemoteSetupFailed(errorMsg);
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
    if (m_engine)
        m_engine->showMessage(progressOutput + QLatin1Char('\n'), Debugger::AppStuff);
}

void QnxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    if (m_engine)
        m_engine->showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void QnxDebugSupport::handleError(const QString &error)
{
    if (state() == Running) {
        if (m_engine) {
            m_engine->showMessage(error, Debugger::AppError);
            m_engine->notifyInferiorIll();
        }
    } else if (state() != Inactive) {
        setFinished();
        if (m_engine)
            m_engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
    }
}

void QnxDebugSupport::printMissingWarning()
{
    if (m_engine)
        m_engine->showMessage(tr("Warning: \"slog2info\" is not found on the device, debug output not available."), Debugger::AppError);
}

void QnxDebugSupport::handleApplicationOutput(const QString &msg, Utils::OutputFormat outputFormat)
{
    Q_UNUSED(outputFormat);
    if (m_engine)
        m_engine->showMessage(msg, Debugger::AppOutput);
}
