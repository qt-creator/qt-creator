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

#include "winrtdebugsupport.h"
#include "winrtrunconfiguration.h"
#include "winrtrunnerhelper.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>

#include <utils/qtcprocess.h>

namespace WinRt {
namespace Internal {

using namespace ProjectExplorer;

WinRtDebugSupport::WinRtDebugSupport(RunControl *runControl, WinRtRunnerHelper *runner)
    : QObject(runControl)
    , m_debugRunControl(runControl)
    , m_runner(runner)
{
    connect(m_debugRunControl, &RunControl::finished, this, &WinRtDebugSupport::finish);
}

bool WinRtDebugSupport::useQmlDebugging(WinRtRunConfiguration *runConfig)
{
    Debugger::DebuggerRunConfigurationAspect *extraAspect =
            runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    return extraAspect && extraAspect->useQmlDebugger();
}

bool WinRtDebugSupport::getFreePort(Utils::Port &qmlDebuggerPort, QString *errorMessage)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost,
                       qmlDebuggerPort.isValid() ? qmlDebuggerPort.number() : 0)) {
        *errorMessage = tr("Not enough free ports for QML debugging.");
        return false;
    }
    qmlDebuggerPort = Utils::Port(server.serverPort());
    return true;
}

WinRtDebugSupport::~WinRtDebugSupport()
{
    delete m_runner;
}

void WinRtDebugSupport::finish()
{
    m_runner->stop();
}

RunControl *WinRtDebugSupport::createDebugRunControl(WinRtRunConfiguration *runConfig,
                                                     Core::Id mode,
                                                     QString *errorMessage)
{
    // FIXME: This is just working for local debugging;
    using namespace Debugger;
    DebuggerStartParameters params;
    params.startMode = AttachExternal;
    // The first Thread needs to be resumed manually.
    params.commandsAfterConnect = "~0 m";

    QFileInfo debuggerHelper(QCoreApplication::applicationDirPath()
                             + QLatin1String("/winrtdebughelper.exe"));
    if (!debuggerHelper.isExecutable()) {
        *errorMessage = tr("The WinRT debugging helper is missing from your Qt Creator "
                           "installation. It was assumed to be located at %1").arg(
                    debuggerHelper.absoluteFilePath());
        return 0;
    }

    if (useQmlDebugging(runConfig)) {
        Utils::Port qmlDebugPort;
        if (!getFreePort(qmlDebugPort, errorMessage))
            return 0;
        params.qmlServer.host = QHostAddress(QHostAddress::LocalHost).toString();
        params.qmlServer.port = qmlDebugPort;
    }

    WinRtRunnerHelper *runner = new WinRtRunnerHelper(runConfig, errorMessage);
    if (!errorMessage->isEmpty())
        return 0;

    QLocalServer server;
    server.listen(QLatin1String("QtCreatorWinRtDebugPIDPipe"));

    runner->debug(debuggerHelper.absoluteFilePath());
    if (!runner->waitForStarted()) {
        *errorMessage = tr("Cannot start the WinRT Runner Tool.");
        return 0;
    }

    if (!server.waitForNewConnection(10000)) {
        *errorMessage = tr("Cannot establish connection to the WinRT debugging helper.");
        return 0;
    }

    while (server.hasPendingConnections()) {
        QLocalSocket *connection = server.nextPendingConnection();
        if (connection->waitForReadyRead(1000)) {
            const QByteArray &output = connection->readAll();
            QList<QByteArray> arg = output.split(':');
            if (arg.first() == "PID") {
                bool ok =false;
                params.attachPID = Utils::ProcessHandle(arg.last().toInt(&ok));
                if (!ok) {
                    *errorMessage = tr("Cannot extract the PID from the WinRT debugging helper. "
                                       "(output: %1)").arg(QString::fromLocal8Bit(output));
                    return 0;
                }
                server.close();
                Debugger::DebuggerRunControl *debugRunControl
                        = createDebuggerRunControl(params, runConfig, errorMessage, mode);
                runner->setDebugRunControl(debugRunControl);
                new WinRtDebugSupport(debugRunControl, runner);
                return debugRunControl;
            }
        }
    }

    server.close();

    *errorMessage = tr("Cannot create an appropriate run control for "
                       "the current run configuration.");

    return 0;
}

} // namespace Internal
} // namespace WinRt
