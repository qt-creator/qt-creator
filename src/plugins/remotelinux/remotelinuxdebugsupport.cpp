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

#include "remotelinuxdebugsupport.h"

#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxrunconfiguration.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("LinuxDeviceDebugSupport");

    m_portsGatherer = new GdbServerPortsGatherer(runControl);
    m_portsGatherer->setUseGdbServer(isCppDebugging());
    m_portsGatherer->setUseQmlServer(isQmlDebugging());

    auto gdbServer = new GdbServerRunner(runControl, m_portsGatherer);
    gdbServer->addDependency(m_portsGatherer);

    addDependency(gdbServer);

    RunConfiguration *runConfig = runControl->runConfiguration();
    if (auto rlrc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig))
        m_symbolFile = rlrc->localExecutableFilePath();
    else if (auto rlrc = qobject_cast<Internal::RemoteLinuxCustomRunConfiguration *>(runConfig))
        m_symbolFile = rlrc->localExecutableFilePath();
}

void LinuxDeviceDebugSupport::start()
{
    if (m_symbolFile.isEmpty()) {
        reportFailure(tr("Cannot debug: Local executable is not set."));
        return;
    }

    const QString host = device()->sshParameters().host;
    const Port gdbServerPort = m_portsGatherer->gdbServerPort();
    const Port qmlServerPort = m_portsGatherer->qmlServerPort();

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.closeMode = KillAndExitMonitorAtClose;

    if (isQmlDebugging()) {
        params.qmlServer.host = host;
        params.qmlServer.port = qmlServerPort;
        params.inferior.commandLineArguments.replace("%qml_port%",
                        QString::number(qmlServerPort.number()));
    }
    if (isCppDebugging()) {
        Runnable r = runnable();
        QTC_ASSERT(r.is<StandardRunnable>(), return);
        auto stdRunnable = r.as<StandardRunnable>();
        params.useExtendedRemote = true;
        params.inferior.executable = stdRunnable.executable;
        params.inferior.commandLineArguments = stdRunnable.commandLineArguments;
        if (isQmlDebugging()) {
            params.inferior.commandLineArguments.prepend(' ');
            params.inferior.commandLineArguments.prepend(
                QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices));
        }

        params.remoteChannel = QString("%1:%2").arg(host).arg(gdbServerPort.number());
        params.symbolFile = m_symbolFile;
    }

    setStartParameters(params);

    DebuggerRunTool::start();
}

} // namespace RemoteLinux
