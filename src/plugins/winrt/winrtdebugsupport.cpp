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

#include <app/app_version.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>

#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

WinRtDebugSupport::WinRtDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    // FIXME: This is just working for local debugging;
    setStartMode(AttachExternal);
    // The first Thread needs to be resumed manually.
    setCommandsAfterConnect("~0 m");

    QFileInfo debuggerHelper(QCoreApplication::applicationDirPath()
                             + QLatin1String("/winrtdebughelper.exe"));
    if (!debuggerHelper.isExecutable()) {
        reportFailure(tr("The WinRT debugging helper is missing from your %1 "
                         "installation. It was assumed to be located at %2")
                      .arg(Core::Constants::IDE_DISPLAY_NAME)
                      .arg(debuggerHelper.absoluteFilePath()));
        return;
    }

    if (isQmlDebugging()) {
        QUrl qmlServer = Utils::urlFromLocalHostAndFreePort();
        if (qmlServer.port() <= 0) {
            reportFailure(tr("Not enough free ports for QML debugging."));
            return;
        }
        setQmlServer(qmlServer);
    }

    QString errorMessage;
    m_runner = new WinRtRunnerHelper(this, &errorMessage);
    if (!errorMessage.isEmpty()) {
        reportFailure(errorMessage);
        return;
    }

    QLocalServer server;
    server.listen(QLatin1String("QtCreatorWinRtDebugPIDPipe"));

    m_runner->debug(debuggerHelper.absoluteFilePath());
    if (!m_runner->waitForStarted()) {
        reportFailure(tr("Cannot start the WinRT Runner Tool."));
        return;
    }

    if (!server.waitForNewConnection(10000)) {
        reportFailure(tr("Cannot establish connection to the WinRT debugging helper."));
        return;
    }

    while (server.hasPendingConnections()) {
        QLocalSocket *connection = server.nextPendingConnection();
        if (connection->waitForReadyRead(1000)) {
            const QByteArray &output = connection->readAll();
            QList<QByteArray> arg = output.split(':');
            if (arg.first() == "PID") {
                bool ok =false;
                int pid = arg.last().toInt(&ok);
                if (!ok) {
                    reportFailure(tr("Cannot extract the PID from the WinRT debugging helper. "
                                     "(output: %1)").arg(QString::fromLocal8Bit(output)));
                    return;
                }
                setAttachPid(Utils::ProcessHandle(pid));
                server.close();
                return;
            }
        }
    }

    server.close();

    reportFailure(tr("Cannot create an appropriate run control for "
                     "the current run configuration."));
}

WinRtDebugSupport::~WinRtDebugSupport()
{
    delete m_runner;
}

} // namespace Internal
} // namespace WinRt
