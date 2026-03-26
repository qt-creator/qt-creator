// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotedebuggerdebugsupport.h"

#include "remotedebuggerconstants.h"
#include "debuggertr.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;
using namespace Debugger;

namespace RemoteDebugger::Internal {

class RemoteDebuggerWorkerFactory final : public RunWorkerFactory
{
public:
    RemoteDebuggerWorkerFactory()
    {
        setId("RemoteDebuggerWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setStartMode(AttachToRemoteServer);
            rp.setCloseMode(KillAndExitMonitorAtClose);
            rp.setUseContinueInsteadOfRun(true);

            const FilePath symbolFile = rp.symbolFile();
            if (!symbolFile.isEmpty()) {
                ProcessRunData inferior;
                inferior.command.setExecutable(symbolFile);
                rp.setInferior(inferior);
            }

            if (auto data = runControl->aspectData(Id{Constants::GdbServerAddressAspectId})) {
                auto stringData = static_cast<const StringAspect::Data *>(data);
                rp.setRemoteChannel(stringData->value);
                rp.setDisplayName(Tr::tr("Attach to %1").arg(rp.remoteChannel()));
            }

            if (auto data = runControl->aspectData(Id{Constants::GdbServerBreakOnMainAspectId})) {
                auto boolData = static_cast<const BoolAspect::Data *>(data);
                rp.setBreakOnMain(boolData->value);
            }

            if (auto data = runControl->aspectData(Id{Constants::GdbServerExtendedModeAspectId})) {
                auto boolData = static_cast<const BoolAspect::Data *>(data);
                rp.setUseExtendedRemote(boolData->value);
            }

            const auto onLauncherSetup = [runControl](Process &process) {
                process.setCommand(runControl->commandLine());
                process.setWorkingDirectory(runControl->workingDirectory());
                process.setEnvironment(runControl->environment());

                QObject::connect(&process, &Process::readyReadStandardOutput, runControl,
                                 [runControl, p = &process] {
                    runControl->postMessage(p->readAllStandardOutput(), StdOutFormat, false);
                });
                QObject::connect(&process, &Process::readyReadStandardError, runControl,
                                 [runControl, p = &process] {
                    runControl->postMessage(p->readAllStandardError(), StdErrFormat, false);
                });

                runControl->postMessage(
                    Tr::tr("Starting %1 ...").arg(process.commandLine().displayName()),
                    NormalMessageFormat);
                return SetupResult::Continue;
            };
            const auto onLauncherDone = [runControl](const Process &process) {
                runControl->postMessage(process.errorString(), ErrorMessageFormat);
            };

            const ProcessTask launcherTask(onLauncherSetup, onLauncherDone, CallDoneFlag::OnError);

            return Group {
                When (launcherTask, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    debuggerRecipe(runControl, rp)
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::RunConfigId);
    }
};

void setupRemoteDebuggerDebugSupport()
{
    static RemoteDebuggerWorkerFactory debugWorkerFactory;
}

} // RemoteDebugger::Internal
