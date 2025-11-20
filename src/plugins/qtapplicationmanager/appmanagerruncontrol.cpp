// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerruncontrol.h"

#include "appmanagerconstants.h"
#include "appmanagerstringaspect.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"
#include "appmanagerutilities.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerruncontrol.h>

#include <perfprofiler/perfprofilerconstants.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/target.h>

#include <solutions/tasking/barrier.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace AppManager::Internal {

static ProcessTask inferiorProcess(RunControl *runControl, QmlDebugServicesPreset qmlServices,
                                   bool suppressDefaultStdOutHandling = false)
{
    const auto modifier = [runControl, qmlServices](Process &process) {
        FilePath controller = runControl->aspectData<AppManagerControllerAspect>()->filePath;
        QString appId = runControl->aspectData<AppManagerIdAspect>()->value;
        QString instanceId = runControl->aspectData<AppManagerInstanceIdAspect>()->value;
        QString documentUrl = runControl->aspectData<AppManagerDocumentUrlAspect>()->value;
        bool restartIfRunning = runControl->aspectData<AppManagerRestartIfRunningAspect>()->value;
        QStringList envVars;
        if (auto envAspect = runControl->aspectData<EnvironmentAspect>())
            envVars = envAspect->environment.toStringList();
        envVars.replaceInStrings(" ", "\\ ");

        CommandLine cmd{controller};
        if (!instanceId.isEmpty())
            cmd.addArgs({"--instance-id", instanceId});

        cmd.addArg("debug-application");

        if (runControl->usesDebugChannel() || runControl->usesQmlChannel()) {
            QStringList debugArgs;
            debugArgs.append(envVars.join(' '));
            if (runControl->usesDebugChannel())
                debugArgs.append(QString("gdbserver :%1").arg(runControl->debugChannel().port()));
            if (runControl->usesQmlChannel()) {
                const QString qmlArgs = qmlDebugCommandLineArguments(qmlServices,
                    QString("port:%1").arg(runControl->qmlChannel().port()), true);
                debugArgs.append(QString("%program% %1 %arguments%") .arg(qmlArgs));
            }
            cmd.addArg(debugArgs.join(' '));
        }
        if (runControl->usesPerfChannel()) {
            const Store perfArgs = runControl->settingsData(PerfProfiler::Constants::PerfSettingsId);
            const QString recordArgs = perfArgs[PerfProfiler::Constants::PerfRecordArgsId].toString();
            cmd.addArg(QString("perf record %1 -o - --").arg(recordArgs));
        }

        cmd.addArg("-eio");
        if (restartIfRunning)
            cmd.addArg("--restart");
        cmd.addArg(appId);

        if (!documentUrl.isEmpty())
            cmd.addArg(documentUrl);

        // Always use the default environment to start the appman-controller in
        // The env variables from the EnvironmentAspect are set through the controller
        process.setEnvironment({});
        // Prevent the write channel to be closed, otherwise the appman-controller will exit
        process.setProcessMode(ProcessMode::Writer);
        process.setCommand(cmd);

        runControl->postMessage(Tr::tr("Starting Application Manager debugging..."), NormalMessageFormat);
        runControl->postMessage(Tr::tr("Using: %1.").arg(cmd.toUserOutput()), NormalMessageFormat);
    };
    return runControl->processTaskWithModifier(modifier, {suppressDefaultStdOutHandling});
}

class AppManagerRunWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerRunWorkerFactory()
    {
        setId("AppManagerRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            const auto modifier = [runControl](Process &process) {
                FilePath controller = runControl->aspectData<AppManagerControllerAspect>()->filePath;
                QString appId = runControl->aspectData<AppManagerIdAspect>()->value;
                QString instanceId = runControl->aspectData<AppManagerInstanceIdAspect>()->value;
                QString documentUrl = runControl->aspectData<AppManagerDocumentUrlAspect>()->value;
                bool restartIfRunning = runControl->aspectData<AppManagerRestartIfRunningAspect>()->value;
                QStringList envVars;
                if (auto envAspect = runControl->aspectData<EnvironmentAspect>())
                    envVars = envAspect->environment.toStringList();
                envVars.replaceInStrings(" ", "\\ ");

                // Always use the default environment to start the appman-controller in
                // The env variables from the EnvironmentAspect are set through the controller
                process.setEnvironment({});
                // Prevent the write channel to be closed, otherwise the appman-controller will exit
                process.setProcessMode(ProcessMode::Writer);
                CommandLine cmd{controller};
                if (!instanceId.isEmpty())
                    cmd.addArgs({"--instance-id", instanceId});

                if (envVars.isEmpty())
                    cmd.addArgs({"start-application", "-eio"});
                else
                    cmd.addArgs({"debug-application", "-eio"});

                if (restartIfRunning)
                    cmd.addArg("--restart");
                if (!envVars.isEmpty())
                    cmd.addArg(envVars.join(' '));
                cmd.addArg(appId);
                if (!documentUrl.isEmpty())
                    cmd.addArg(documentUrl);
                process.setCommand(cmd);
            };
            return runControl->processRecipe(modifier);
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::RUNCONFIGURATION_ID);
        addSupportedRunConfig(Constants::RUNANDDEBUGCONFIGURATION_ID);
    }
};

class AppManagerDebugWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerDebugWorkerFactory()
    {
        setId("AppManagerDebugWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            BuildConfiguration *bc = runControl->buildConfiguration();

            const Internal::TargetInformation targetInformation(bc);
            if (!targetInformation.isValid())
                return runControl->errorTask(Tr::tr("Cannot debug: Invalid target information."));

            FilePath symbolFile;

            if (targetInformation.manifest.isQmlRuntime()) {
                symbolFile = getToolFilePath(Constants::APPMAN_LAUNCHER_QML,
                                             runControl->kit(),
                                             RunDeviceKitAspect::device(runControl->kit()));
            } else if (targetInformation.manifest.isNativeRuntime()) {
                symbolFile = Utils::findOrDefault(bc->buildSystem()->applicationTargets(),
                                                  [&](const BuildTargetInfo &ti) {
                                                      return ti.buildKey == targetInformation.manifest.code
                                                             || ti.projectFilePath.toUrlishString() == targetInformation.manifest.code;
                                                  }).targetFilePath;
            } else {
                return runControl->errorTask(
                    Tr::tr("Cannot debug: Only QML and native applications are supported."));
            }
            if (symbolFile.isEmpty()) {
                return runControl->errorTask(Tr::tr("Cannot debug: Local executable is not set."));
            }

            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setupPortsGatherer(runControl);
            rp.setStartMode(Debugger::AttachToRemoteServer);
            rp.setCloseMode(Debugger::KillAndExitMonitorAtClose);
            rp.setSkipDebugServer(true);

            if (rp.isCppDebugging()) {
                rp.setUseExtendedRemote(false);
                rp.setUseContinueInsteadOfRun(true);
                rp.setContinueAfterAttach(true);
                rp.setSymbolFile(symbolFile);

                QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(runControl->kit());
                if (version) {
                    rp.setSolibSearchPath(version->qtSoPaths());
                    rp.addSearchDirectory(version->qmlPath());
                }

                const FilePath sysroot = SysRootKitAspect::sysRoot(runControl->kit());
                if (sysroot.isEmpty())
                    rp.setSysRoot("/");
                else
                    rp.setSysRoot(sysroot);
            }

            const ProcessTask inferior(inferiorProcess(runControl, QmlDebuggerServices));
            return {
                When (inferior, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    debuggerRecipe(runControl, rp)
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::RUNANDDEBUGCONFIGURATION_ID);
    }
};

class AppManagerQmlToolingWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerQmlToolingWorkerFactory()
    {
        setId("AppManagerQmlToolingWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();
            const ProcessTask inferior(inferiorProcess(runControl,
                                                       servicesForRunMode(runControl->runMode())));
            return Group {
                When (inferior, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    runControl->createRecipe(runnerIdForRunMode(runControl->runMode()))
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedRunConfig(Constants::RUNANDDEBUGCONFIGURATION_ID);
    }
};

class AppManagerPerfProfilerWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerPerfProfilerWorkerFactory()
    {
        setId("AppManagerPerfProfilerWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestPerfChannel();
            return runControl->processRecipe(inferiorProcess(runControl, NoQmlDebugServices, true));
        });
        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUNNER);
        addSupportedRunConfig(Constants::RUNANDDEBUGCONFIGURATION_ID);
    }
};

void setupAppManagerRunWorker()
{
    static AppManagerRunWorkerFactory theAppManagerRunWorkerFactory;
}

void setupAppManagerDebugWorker()
{
    static AppManagerDebugWorkerFactory theAppManagerDebugWorkerFactory;
}

void setupAppManagerQmlToolingWorker()
{
    static AppManagerQmlToolingWorkerFactory theAppManagerQmlToolingWorkerFactory;
}

void setupAppManagerPerfProfilerWorker()
{
    static AppManagerPerfProfilerWorkerFactory theAppManagerPerfProfilerWorkerFactory;
}

} // AppManager::Internal
