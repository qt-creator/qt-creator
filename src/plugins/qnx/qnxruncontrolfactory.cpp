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

#include "qnxruncontrolfactory.h"
#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdebugsupport.h"
#include "qnxanalyzesupport.h"
#include "qnxqtversion.h"
#include "qnxruncontrol.h"
#include "qnxutils.h"
#include "qnxdeviceconfiguration.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/portlist.h>

using namespace Analyzer;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

static DebuggerStartParameters createDebuggerStartParameters(QnxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();

    const IDevice::ConstPtr device = DeviceKitInformation::device(k);
    if (device.isNull())
        return params;

    params.startMode = AttachToRemoteServer;
    params.useCtrlCStub = true;
    params.inferior.executable = runConfig->localExecutableFilePath();
    params.remoteExecutable = runConfig->remoteExecutableFilePath();
    params.remoteChannel = device->sshParameters().host + QLatin1String(":-1");
    params.remoteSetupNeeded = true;
    params.closeMode = KillAtClose;
    params.inferior.commandLineArguments = runConfig->arguments();

    auto aspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        params.qmlServerAddress = device->sshParameters().host;
        params.qmlServerPort = 0; // QML port is handed out later
    }

    auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(k));
    if (qtVersion)
        params.solibSearchPath = QnxUtils::searchPaths(qtVersion);

    return params;
}

QnxRunControlFactory::QnxRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool QnxRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        return false;
    }

    if (!runConfiguration->isEnabled()
            || !runConfiguration->id().name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX)) {
        return false;
    }

    const QnxDeviceConfiguration::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit())
            .dynamicCast<const QnxDeviceConfiguration>();
    if (dev.isNull())
        return false;

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
            || mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        auto aspect = runConfiguration->extraAspect<DebuggerRunConfigurationAspect>();
        int portsUsed = aspect ? aspect->portsUsedByDebugger() : 0;
        return portsUsed <= dev->freePorts().count();
    }

    return true;
}

RunControl *QnxRunControlFactory::create(RunConfiguration *runConfig, Core::Id mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);
    auto rc = qobject_cast<QnxRunConfiguration *>(runConfig);
    QTC_ASSERT(rc, return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return new QnxRunControl(rc);

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE) {
        const DebuggerStartParameters params = createDebuggerStartParameters(rc);
        DebuggerRunControl *runControl = createDebuggerRunControl(params, runConfig, errorMessage);
        QTC_ASSERT(runControl, return 0);
        auto debugSupport = new QnxDebugSupport(rc, runControl);
        connect(runControl, &RunControl::finished, debugSupport, &QnxDebugSupport::handleDebuggingFinished);
        return runControl;
    }

    if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        Kit *kit = runConfig->target()->kit();
        const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
        if (device.isNull())
            return 0;
        AnalyzerRunControl *runControl = AnalyzerManager::createRunControl(runConfig, mode);
        QTC_ASSERT(runControl, return 0);
        runControl->setRunnable(runConfig->runnable());
        AnalyzerConnection connection;
        connection.connParams = device->sshParameters();
        connection.analyzerHost = connection.connParams.host;
        connection.analyzerPort = connection.connParams.port;
        runControl->setConnection(connection);
        auto analyzeSupport = new QnxAnalyzeSupport(rc, runControl);
        connect(runControl, &RunControl::finished, analyzeSupport, &QnxAnalyzeSupport::handleProfilingFinished);
        return runControl;
    }

    QTC_CHECK(false);
    return 0;
}
