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

#include "remotelinuxruncontrolfactory.h"

#include "remotelinuxanalyzesupport.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxruncontrol.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <debugger/analyzer/analyzerstartparameters.h>

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxRunControlFactory::RemoteLinuxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool RemoteLinuxRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        return false;
    }

    const Core::Id id = runConfiguration->id();
    return runConfiguration->isEnabled()
            && (id == RemoteLinuxCustomRunConfiguration::runConfigId()
                || id.name().startsWith(RemoteLinuxRunConfiguration::IdPrefix));
}

RunControl *RemoteLinuxRunControlFactory::create(RunConfiguration *runConfig, Core::Id mode,
                                                 QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return new RemoteLinuxRunControl(runConfig);

    const auto rcRunnable = runConfig->runnable();
    QTC_ASSERT(rcRunnable.is<StandardRunnable>(), return 0);
    const auto stdRunnable = rcRunnable.as<StandardRunnable>();

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
            || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {
        IDevice::ConstPtr dev = DeviceKitInformation::device(runConfig->target()->kit());
        if (!dev) {
            *errorMessage = tr("Cannot debug: Kit has no device.");
            return 0;
        }

        auto aspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
        int portsUsed = aspect ? aspect->portsUsedByDebugger() : 0;
        if (portsUsed > dev->freePorts().count()) {
            *errorMessage = tr("Cannot debug: Not enough free ports available.");
            return 0;
        }

        QString localExecutable;
        if (auto rlrc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig))
            localExecutable = rlrc->localExecutableFilePath();
        if (localExecutable.isEmpty()) {
            *errorMessage = tr("Cannot debug: Local executable is not set.");
            return 0;
        }

        DebuggerStartParameters params;
        params.startMode = AttachToRemoteServer;
        params.closeMode = KillAndExitMonitorAtClose;
        params.remoteSetupNeeded = true;

        if (aspect->useQmlDebugger()) {
            params.qmlServerAddress = dev->sshParameters().host;
            params.qmlServerPort = 0; // port is selected later on
        }
        if (aspect->useCppDebugger()) {
            aspect->setUseMultiProcess(true);
            params.inferior.commandLineArguments = stdRunnable.commandLineArguments;
            if (aspect->useQmlDebugger()) {
                params.inferior.commandLineArguments.prepend(QLatin1Char(' '));
                params.inferior.commandLineArguments.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices));
            }
            params.inferior.commandLineArguments = localExecutable;
            params.remoteChannel = dev->sshParameters().host + QLatin1String(":-1");
            params.remoteExecutable = stdRunnable.executable;
        }

        DebuggerRunControl * const runControl = createDebuggerRunControl(params, runConfig, errorMessage, mode);
        if (!runControl)
            return 0;
        (void) new LinuxDeviceDebugSupport(runConfig, runControl);
        return runControl;
    }

    if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        auto runControl = AnalyzerManager::createRunControl(runConfig, mode);
        AnalyzerConnection connection;
        connection.connParams =
            DeviceKitInformation::device(runConfig->target()->kit())->sshParameters();
        connection.analyzerHost = connection.connParams.host;
        runControl->setConnection(connection);
        (void) new RemoteLinuxAnalyzeSupport(runConfig, runControl, mode);
        return runControl;
    }

    QTC_CHECK(false);
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
