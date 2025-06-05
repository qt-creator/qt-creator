// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevicedebugsupport.h"

#include "qdbconstants.h"

#include <debugger/debuggerruncontrol.h>

#include <perfprofiler/perfprofilerconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <solutions/tasking/barrier.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Qdb::Internal {

static ProcessTask qdbDeviceInferiorProcess(RunControl *runControl,
                                            QmlDebugServicesPreset qmlServices,
                                            bool suppressDefaultStdOutHandling = false)
{
    const auto modifier = [runControl, qmlServices](Process &process) {
        CommandLine cmd{runControl->device()->filePath(Constants::AppcontrollerFilepath)};

        int lowerPort = 0;
        int upperPort = 0;

        if (runControl->usesDebugChannel()) {
            cmd.addArg("--debug-gdb");
            lowerPort = upperPort = runControl->debugChannel().port();
        }
        if (runControl->usesQmlChannel()) {
            cmd.addArg("--debug-qml");
            cmd.addArg("--qml-debug-services");
            cmd.addArg(qmlDebugServices(qmlServices));
            lowerPort = upperPort = runControl->qmlChannel().port();
        }
        if (runControl->usesDebugChannel() && runControl->usesQmlChannel()) {
            lowerPort = runControl->debugChannel().port();
            upperPort = runControl->qmlChannel().port();
            if (lowerPort + 1 != upperPort) {
                runControl->postMessage("Need adjacent free ports for combined C++/QML debugging",
                                        ErrorMessageFormat);
                return Tasking::SetupResult::StopWithError;
            }
        }
        if (runControl->usesPerfChannel()) {
            const Store perfArgs = runControl->settingsData(PerfProfiler::Constants::PerfSettingsId);
            // appcontroller is not very clear about this, but it expects a comma-separated list of arguments.
            // Any literal commas that apper in the args should be escaped by additional commas.
            // See the source at
            // https://code.qt.io/cgit/qt-apps/boot2qt-appcontroller.git/tree/main.cpp?id=658dc91cf561e41704619a55fbb1f708decf134e#n434
            // and adjust if necessary.
            const QString recordArgs = perfArgs[PerfProfiler::Constants::PerfRecordArgsId]
                                           .toString()
                                           .replace(',', ",,")
                                           .split(' ', Qt::SkipEmptyParts)
                                           .join(',');
            cmd.addArg("--profile-perf");
            cmd.addArgs(recordArgs, CommandLine::Raw);
            lowerPort = upperPort = runControl->perfChannel().port();
        }

        cmd.addArg("--port-range");
        cmd.addArg(QString("%1-%2").arg(lowerPort).arg(upperPort));
        cmd.addCommandLineAsArgs(runControl->commandLine());

        process.setCommand(cmd);
        process.setWorkingDirectory(runControl->workingDirectory());
        process.setEnvironment(runControl->environment());
        return Tasking::SetupResult::Continue;
    };
    return processTaskWithModifier(runControl, modifier, suppressDefaultStdOutHandling);
}

static RunWorker *createQdbDeviceInferiorWorker(RunControl *runControl,
                                                QmlDebugServicesPreset qmlServices,
                                                bool suppressDefaultStdOutHandling = false)
{
    return new RunWorker(runControl, { processRecipe(qdbDeviceInferiorProcess(
                                         runControl, qmlServices, suppressDefaultStdOutHandling)) });
}

class QdbRunWorkerFactory final : public RunWorkerFactory
{
public:
    QdbRunWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            const auto modifier = [runControl](Process &process) {
                const CommandLine remoteCommand = runControl->commandLine();
                const FilePath remoteExe = remoteCommand.executable();
                CommandLine cmd{remoteExe.withNewPath(Constants::AppcontrollerFilepath)};
                cmd.addArg(remoteExe.nativePath());
                cmd.addArgs(remoteCommand.arguments(), CommandLine::Raw);
                process.setCommand(cmd);
            };
            return createProcessWorker(runControl, modifier);
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::QdbRunConfigurationId);
        addSupportedRunConfig(QmlProjectManager::Constants::QML_RUNCONFIG_ID);
        addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    }
};

class QdbDebugWorkerFactory final : public RunWorkerFactory
{
public:
    QdbDebugWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setupPortsGatherer(runControl);
            rp.setStartMode(Debugger::AttachToRemoteServer);
            rp.setCloseMode(KillAndExitMonitorAtClose);
            rp.setUseContinueInsteadOfRun(true);
            rp.setContinueAfterAttach(true);
            rp.addSolibSearchDir("%{sysroot}/system/lib");
            rp.setSkipDebugServer(true);

            const ProcessTask processTask(qdbDeviceInferiorProcess(runControl, QmlDebuggerServices));
            const Group recipe {
                When (processTask, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    debuggerRecipe(runControl, rp)
                }
            };
            return new RunWorker(runControl, recipe);
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::QdbRunConfigurationId);
        addSupportedRunConfig(QmlProjectManager::Constants::QML_RUNCONFIG_ID);
        addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    }
};

class QdbQmlToolingWorkerFactory final : public RunWorkerFactory
{
public:
    QdbQmlToolingWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();
            const QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());
            auto worker = createQdbDeviceInferiorWorker(runControl, services);

            auto extraWorker = runControl->createWorker(runnerIdForRunMode(runControl->runMode()));
            extraWorker->addStartDependency(worker);
            worker->addStopDependency(extraWorker);
            return worker;
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedRunConfig(Constants::QdbRunConfigurationId);
        addSupportedRunConfig(QmlProjectManager::Constants::QML_RUNCONFIG_ID);
        addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    }
};

class QdbPerfProfilerWorkerFactory final : public RunWorkerFactory
{
public:
    QdbPerfProfilerWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            runControl->requestPerfChannel();
            return createQdbDeviceInferiorWorker(runControl, NoQmlDebugServices, true);
        });
        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUNNER);
        addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
        addSupportedRunConfig(Constants::QdbRunConfigurationId);
    }
};

void setupQdbRunWorkers()
{
    static QdbRunWorkerFactory theQdbRunWorkerFactory;
    static QdbDebugWorkerFactory theQdbDebugWorkerFactory;
    static QdbQmlToolingWorkerFactory theQdbQmlToolingWorkerFactory;
    static QdbPerfProfilerWorkerFactory theQdbProfilerWorkerFactory;
}

} // Qdb::Internal
