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

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>

#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

WinRtDebugSupport::WinRtDebugSupport(RunControl *runControl, QString *errorMessage)
    : DebuggerRunTool(runControl)
{
    // FIXME: This is just working for local debugging;
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
        return;
    }

    if (isQmlDebugging()) {
        Utils::Port qmlDebugPort;
        if (!getFreePort(qmlDebugPort, errorMessage))
            return;
        params.qmlServer.host = QHostAddress(QHostAddress::LocalHost).toString();
        params.qmlServer.port = qmlDebugPort;
    }

    m_runner = new WinRtRunnerHelper(this, errorMessage);
    if (!errorMessage->isEmpty())
        return;

    QLocalServer server;
    server.listen(QLatin1String("QtCreatorWinRtDebugPIDPipe"));

    m_runner->debug(debuggerHelper.absoluteFilePath());
    if (!m_runner->waitForStarted()) {
        *errorMessage = tr("Cannot start the WinRT Runner Tool.");
        return;
    }

    if (!server.waitForNewConnection(10000)) {
        *errorMessage = tr("Cannot establish connection to the WinRT debugging helper.");
        return;
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
                    return;
                }
                server.close();
                setStartParameters(params, errorMessage);
                return;
            }
        }
    }

    server.close();

    *errorMessage = tr("Cannot create an appropriate run control for "
                       "the current run configuration.");
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

} // namespace Internal
} // namespace WinRt
