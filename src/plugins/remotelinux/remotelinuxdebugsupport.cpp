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

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunControl *runControl, QString *errorMessage)
    : DebuggerRunTool(runControl)
{
    RunConfiguration *runConfig = runControl->runConfiguration();

    QString symbolFile;
    if (auto rlrc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig))
        symbolFile = rlrc->localExecutableFilePath();
    if (auto rlrc = qobject_cast<Internal::RemoteLinuxCustomRunConfiguration *>(runConfig))
        symbolFile = rlrc->localExecutableFilePath();
    if (symbolFile.isEmpty()) {
        *errorMessage = tr("Cannot debug: Local executable is not set.");
        return;
    }

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.closeMode = KillAndExitMonitorAtClose;
    params.remoteSetupNeeded = false;

    if (isQmlDebugging()) {
        params.qmlServer.host = device()->sshParameters().host;
        params.qmlServer.port = Utils::Port(); // port is selected later on
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
        params.remoteChannel = device()->sshParameters().host + ":-1";
        params.symbolFile = symbolFile;
    }

    setStartParameters(params);
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
}

void LinuxDeviceDebugSupport::prepare()
{
    auto targetRunner = qobject_cast<AbstractRemoteLinuxRunSupport *>(runControl()->targetRunner());

    if (isCppDebugging()) {
        m_gdbServerPort = targetRunner->findPort();
        if (!m_gdbServerPort.isValid()) {
            reportFailure(tr("Not enough free ports on device for C++ debugging."));
            return;
        }
    }
    if (isQmlDebugging()) {
        m_qmlPort = targetRunner->findPort();
        if (!m_qmlPort.isValid()) {
            reportFailure(tr("Not enough free ports on device for QML debugging."));
            return;
        }
    }

    runControl()->setRunnable(realRunnable());

    RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = m_gdbServerPort;
    result.qmlServerPort = m_qmlPort;
    setRemoteParameters(result);

    DebuggerRunTool::prepare();
}

Runnable LinuxDeviceDebugSupport::realRunnable() const
{
    StandardRunnable r = runControl()->runnable().as<StandardRunnable>();
    QStringList args = QtcProcess::splitArgs(r.commandLineArguments, OsTypeLinux);
    QString command;

    if (isQmlDebugging())
        args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices, m_qmlPort));

    if (isQmlDebugging() && !isCppDebugging()) {
        command = r.executable;
    } else {
        command = device()->debugServerPath();
        if (command.isEmpty())
            command = QLatin1String("gdbserver");
        args.clear();
        args.append(QString::fromLatin1("--multi"));
        args.append(QString::fromLatin1(":%1").arg(m_gdbServerPort.number()));
    }
    r.executable = command;
    r.commandLineArguments = QtcProcess::joinArgs(args, OsTypeLinux);
    return r;
}

} // namespace RemoteLinux
