/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
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

#include "baremetalruncontrolfactory.h"
#include "baremetalgdbcommandsdeploystep.h"
#include "baremetalrunconfiguration.h"
#include "baremetaldevice.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QApplication>

using namespace Analyzer;
using namespace Debugger;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalRunControlFactory::BareMetalRunControlFactory(QObject *parent) :
    ProjectExplorer::IRunControlFactory(parent)
{
}

BareMetalRunControlFactory::~BareMetalRunControlFactory()
{
}

bool BareMetalRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != NormalRunMode && mode != DebugRunMode && mode != DebugRunModeWithBreakOnMain
            && mode != QmlProfilerRunMode) {
        return false;
    }

    const QByteArray idStr = runConfiguration->id().name();
    return runConfiguration->isEnabled() && idStr.startsWith(BareMetalRunConfiguration::IdPrefix);
}

DebuggerStartParameters BareMetalRunControlFactory::startParameters(const BareMetalRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();
    const BareMetalDevice::ConstPtr device = qSharedPointerCast<const BareMetalDevice>(DeviceKitInformation::device(k));
    QTC_ASSERT(device, return params);
    params.sysRoot = SysRootKitInformation::sysRoot(k).toString();
    params.debuggerCommand = DebuggerKitInformation::debuggerCommand(k).toString();
    if (ToolChain *tc = ToolChainKitInformation::toolChain(k))
        params.toolChainAbi = tc->targetAbi();
    params.languages |= CppLanguage;
    params.processArgs = runConfig->arguments();
    params.startMode = AttachToRemoteServer;
    params.executable = runConfig->localExecutableFilePath();
    params.remoteSetupNeeded = false; //FIXME probably start debugserver with that, how?
    params.displayName = runConfig->displayName();
    if (const Project *project = target->project()) {
        params.projectSourceDirectory = project->projectDirectory().toString();
        if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            params.projectBuildDirectory = buildConfig->buildDirectory().toString();
        params.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
    }
    if (device->sshParameters().host.startsWith(QLatin1Char('|'))) //gdb pipe mode enabled
        params.remoteChannel = device->sshParameters().host;
    else
        params.remoteChannel = device->sshParameters().host + QLatin1Char(':') + QString::number(device->sshParameters().port);
    params.remoteSetupNeeded = false; // qml stuff, not needed
    params.commandsAfterConnect = device->gdbInitCommands().toLatin1();
    params.commandsForReset = device->gdbResetCommands().toLatin1();
    BuildConfiguration *bc = target->activeBuildConfiguration();
    BuildStepList *bsl = bc->stepList(BareMetalGdbCommandsDeployStep::stepId());
    if (bsl) {
        foreach (BuildStep *bs, bsl->steps()) {
            BareMetalGdbCommandsDeployStep *ds = qobject_cast<BareMetalGdbCommandsDeployStep *>(bs);
            if (ds) {
                if (!params.commandsAfterConnect.endsWith("\n"))
                    params.commandsAfterConnect.append("\n");
                params.commandsAfterConnect.append(ds->gdbCommands().toLatin1());
            }
        }
    }
    params.useContinueInsteadOfRun = true; //we can't execute as its always running
    return params;
}

RunControl *BareMetalRunControlFactory::create(RunConfiguration *runConfiguration,
                                               RunMode mode,
                                               QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    BareMetalRunConfiguration *rc = qobject_cast<BareMetalRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return 0);
    IDevice::ConstPtr dev = DeviceKitInformation::device(rc->target()->kit());
    if (!dev) {
        *errorMessage = tr("Cannot debug: Kit has no device.");
        return 0;
    }
    DebuggerStartParameters sp = startParameters(rc);
    if (!QFile::exists(sp.executable)) {
        *errorMessage = QApplication::translate("Core::Internal::ExecuteFilter",
                                                "Could not find executable for \"%1\".")
                .arg(sp.executable);
        return 0;
    }
    return DebuggerPlugin::createDebugger(sp,runConfiguration,errorMessage);
}

} // namespace Internal
} // namespace BareMetal
