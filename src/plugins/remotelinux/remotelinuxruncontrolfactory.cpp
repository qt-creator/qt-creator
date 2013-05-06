/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "remotelinuxruncontrolfactory.h"

#include "remotelinuxanalyzesupport.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxruncontrol.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerstartparameters.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
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

RemoteLinuxRunControlFactory::~RemoteLinuxRunControlFactory()
{
}

bool RemoteLinuxRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode && mode != DebugRunModeWithBreakOnMain
            && mode != QmlProfilerRunMode) {
        return false;
    }

    const QByteArray idStr = runConfiguration->id().name();
    return runConfiguration->isEnabled() && idStr.startsWith(RemoteLinuxRunConfiguration::IdPrefix);
}

RunControl *RemoteLinuxRunControlFactory::create(RunConfiguration *runConfig, RunMode mode,
                                                 QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);

    RemoteLinuxRunConfiguration *rc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig);
    QTC_ASSERT(rc, return 0);
    switch (mode) {
    case NormalRunMode:
        return new RemoteLinuxRunControl(rc);
    case DebugRunMode:
    case DebugRunModeWithBreakOnMain: {
        IDevice::ConstPtr dev = DeviceKitInformation::device(rc->target()->kit());
        if (!dev) {
            *errorMessage = tr("Cannot debug: Kit has no device.");
            return 0;
        }
        if (rc->portsUsedByDebuggers() > dev->freePorts().count()) {
            *errorMessage = tr("Cannot debug: Not enough free ports available.");
            return 0;
        }
        DebuggerStartParameters params = LinuxDeviceDebugSupport::startParameters(rc);
        if (mode == ProjectExplorer::DebugRunModeWithBreakOnMain)
            params.breakOnMain = true;
        DebuggerRunControl * const runControl
                = DebuggerPlugin::createDebugger(params, rc, errorMessage);
        if (!runControl)
            return 0;
        LinuxDeviceDebugSupport * const debugSupport =
                new LinuxDeviceDebugSupport(rc, runControl->engine());
        connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
        return runControl;
    }
    case QmlProfilerRunMode: {
        IAnalyzerTool *tool = AnalyzerManager::toolFromRunMode(mode);
        if (!tool) {
            if (errorMessage)
                *errorMessage = tr("No analyzer tool selected.");
            return 0;
        }
        AnalyzerStartParameters params = RemoteLinuxAnalyzeSupport::startParameters(rc, mode);
        AnalyzerRunControl * const runControl = new AnalyzerRunControl(tool, params, runConfig);
        RemoteLinuxAnalyzeSupport * const analyzeSupport =
                new RemoteLinuxAnalyzeSupport(rc, runControl->engine(), mode);
        connect(runControl, SIGNAL(finished()), analyzeSupport, SLOT(handleProfilingFinished()));
        return runControl;
    }
    case NoRunMode:
    case CallgrindRunMode:
    case MemcheckRunMode:
        QTC_ASSERT(false, return 0);
    }

    QTC_ASSERT(false, return 0);
}

} // namespace Internal
} // namespace RemoteLinux
