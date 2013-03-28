/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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
#include "qnxrunconfiguration.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

using namespace Qnx;
using namespace Qnx::Internal;

QnxDebugSupport::QnxDebugSupport(QnxRunConfiguration *runConfig, Debugger::DebuggerEngine *engine)
    : QObject(engine)
    , m_remoteExecutable(runConfig->remoteExecutableFilePath())
    , m_commandPrefix(runConfig->commandPrefix())
    , m_device(DeviceKitInformation::device(runConfig->target()->kit()))
    , m_engine(engine)
    , m_pdebugPort(-1)
    , m_qmlPort(-1)
    , m_useCppDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger())
    , m_useQmlDebugger(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger())
    , m_state(Inactive)
{
    m_runner = new DeviceApplicationRunner(this);
    m_portsGatherer = new DeviceUsedPortsGatherer(this);

    connect(m_portsGatherer, SIGNAL(error(QString)), SLOT(handleError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));

    connect(m_runner, SIGNAL(reportError(QString)), SLOT(handleError(QString)));
    connect(m_runner, SIGNAL(remoteProcessStarted()),        this, SLOT(handleRemoteProcessStarted()));
    connect(m_runner, SIGNAL(finished(bool)), SLOT(handleRemoteProcessFinished(bool)));
    connect(m_runner, SIGNAL(reportProgress(QString)),       this, SLOT(handleProgressReport(QString)));
    connect(m_runner, SIGNAL(remoteStdout(QByteArray)),      this, SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteStderr(QByteArray)),     this, SLOT(handleRemoteOutput(QByteArray)));

    connect(m_engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleAdapterSetupRequested()));
}

void QnxDebugSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(m_state == Inactive, return);

    m_state = GatheringPorts;
    if (m_engine)
        m_engine->showMessage(tr("Preparing remote side...\n"), Debugger::AppStuff);
    m_portsGatherer->start(m_device);
}

void QnxDebugSupport::handlePortListReady()
{
    QTC_ASSERT(m_state == GatheringPorts, return);
    startExecution();
}

void QnxDebugSupport::startExecution()
{
    if (m_state == Inactive)
        return;

    Utils::PortList portList = m_device->freePorts();
    if (m_useCppDebugger)
        m_pdebugPort = m_portsGatherer->getNextFreePort(&portList);
    if (m_useQmlDebugger)
        m_qmlPort = m_portsGatherer->getNextFreePort(&portList);

    if ((m_useCppDebugger && m_pdebugPort == -1) || (m_useQmlDebugger && m_qmlPort == -1)) {
        handleError(tr("Not enough free ports on device for debugging."));
        return;
    }

    m_state = StartingRemoteProcess;

    if (m_useQmlDebugger)
        m_engine->startParameters().processArgs += QString::fromLocal8Bit(" -qmljsdebugger=port:%1,block").arg(m_qmlPort);

    QString remoteCommandLine;
    if (m_useCppDebugger)
        remoteCommandLine = QString::fromLatin1("%1 %2 %3")
                .arg(m_commandPrefix, executable()).arg(m_pdebugPort);
    else if (m_useQmlDebugger && !m_useCppDebugger)
        remoteCommandLine = QString::fromLatin1("%1 %2 %3")
                .arg(m_commandPrefix, executable(), m_engine->startParameters().processArgs);

    m_runner->start(m_device, remoteCommandLine.toUtf8());
}

void QnxDebugSupport::handleRemoteProcessStarted()
{
    m_state = Debugging;
    if (m_engine)
        m_engine->notifyEngineRemoteSetupDone(m_pdebugPort, m_qmlPort);
}

void QnxDebugSupport::handleRemoteProcessFinished(bool success)
{
    if (m_engine || m_state == Inactive)
        return;

    if (m_state == Debugging) {
        if (!success)
            m_engine->notifyInferiorIll();

    } else {
        const QString errorMsg = tr("The %1 process closed unexpectedly.").arg(executable());
        m_engine->notifyEngineRemoteSetupFailed(errorMsg);
    }
}

void QnxDebugSupport::handleDebuggingFinished()
{
    setFinished();
}

void QnxDebugSupport::setFinished()
{
    if (m_state != GatheringPorts && m_state != Inactive)
        m_runner->stop(m_device->processSupport()->killProcessByNameCommandLine(executable()).toUtf8());

    m_state = Inactive;
}

QString QnxDebugSupport::executable() const
{
    return m_useCppDebugger? QLatin1String(Constants::QNX_DEBUG_EXECUTABLE) : m_remoteExecutable;
}

void QnxDebugSupport::handleProgressReport(const QString &progressOutput)
{
    if (m_engine)
        m_engine->showMessage(progressOutput + QLatin1Char('\n'), Debugger::AppStuff);
}

void QnxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(m_state == Inactive || m_state == Debugging, return);

    if (m_engine)
        m_engine->showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void QnxDebugSupport::handleError(const QString &error)
{
    if (m_state == Debugging) {
        if (m_engine) {
            m_engine->showMessage(error, Debugger::AppError);
            m_engine->notifyInferiorIll();
        }
    } else if (m_state != Inactive) {
        setFinished();
        if (m_engine)
            m_engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
    }
}
