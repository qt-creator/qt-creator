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
#include <debugger/debuggerkitaspect.h>

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
#include <projectexplorer/toolchain.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

static RunWorker *createInferiorRunner(RunControl *runControl, QmlDebugServicesPreset qmlServices)
{
    auto worker = new SimpleTargetRunner(runControl);
    worker->setId(AppManager::Constants::DEBUG_LAUNCHER_ID);
    worker->setEssential(true);

    if (worker->usesPerfChannel()) {
        worker->suppressDefaultStdOutHandling();
        runControl->setProperty("PerfProcess", QVariant::fromValue(worker->process()));
    }

    worker->setStartModifier([worker, runControl, qmlServices] {
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

        if (worker->usesDebugChannel() || worker->usesQmlChannel()) {
            QStringList debugArgs;
            debugArgs.append(envVars.join(' '));
            if (worker->usesDebugChannel())
                debugArgs.append(QString("gdbserver :%1").arg(worker->debugChannel().port()));
            if (worker->usesQmlChannel()) {
                const QString qmlArgs = qmlDebugCommandLineArguments(qmlServices,
                    QString("port:%1").arg(worker->qmlChannel().port()), true);
                debugArgs.append(QString("%program% %1 %arguments%") .arg(qmlArgs));
            }
            cmd.addArg(debugArgs.join(' '));
        }
        if (worker->usesPerfChannel()) {
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
        worker->setEnvironment({});
        // Prevent the write channel to be closed, otherwise the appman-controller will exit
        worker->setProcessMode(ProcessMode::Writer);
        worker->setCommandLine(cmd);

        worker->appendMessage(Tr::tr("Starting Application Manager debugging..."), NormalMessageFormat);
        worker->appendMessage(Tr::tr("Using: %1.").arg(cmd.toUserOutput()), NormalMessageFormat);
    });
    return worker;
}

// AppManagerDebugSupport

class AppManagerDebugSupport final : public Debugger::DebuggerRunTool
{
public:
    AppManagerDebugSupport(RunControl *runControl)
        : DebuggerRunTool(runControl)
    {
        setId("ApplicationManagerPlugin.Debug.Support");

        if (isCppDebugging())
            runControl->requestDebugChannel();
        if (isQmlDebugging())
            runControl->requestQmlChannel();

        auto debuggee = createInferiorRunner(runControl, QmlDebuggerServices);

        addStartDependency(debuggee);
        addStopDependency(debuggee);
    }

private:
    void start() override
    {
        Target *target = runControl()->target();

        const Internal::TargetInformation targetInformation(target);
        if (!targetInformation.isValid()) {
            reportFailure(Tr::tr("Cannot debug: Invalid target information"));
            return;
        }

        FilePath symbolFile;

        if (targetInformation.manifest.isQmlRuntime()) {
            symbolFile = getToolFilePath(Constants::APPMAN_LAUNCHER_QML,
                                         target->kit(),
                                         RunDeviceKitAspect::device(target->kit()));
        } else if (targetInformation.manifest.isNativeRuntime()) {
            symbolFile = Utils::findOrDefault(target->buildSystem()->applicationTargets(),
               [&](const BuildTargetInfo &ti) {
                   return ti.buildKey == targetInformation.manifest.code
                       || ti.projectFilePath.toString() == targetInformation.manifest.code;
               }).targetFilePath;
        } else {
            reportFailure(Tr::tr("Cannot debug: Only QML and native applications are supported."));
        }
        if (symbolFile.isEmpty()) {
            reportFailure(Tr::tr("Cannot debug: Local executable is not set."));
            return;
        }

        setStartMode(Debugger::AttachToRemoteServer);
        setCloseMode(Debugger::KillAndExitMonitorAtClose);

        if (isQmlDebugging())
            setQmlServer(qmlChannel());

        if (isCppDebugging()) {
            setUseExtendedRemote(false);
            setUseContinueInsteadOfRun(true);
            setContinueAfterAttach(true);
            setRemoteChannel(debugChannel());
            setSymbolFile(symbolFile);

            QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(runControl()->kit());
            if (version) {
                setSolibSearchPath(version->qtSoPaths());
                addSearchDirectory(version->qmlPath());
            }

            auto sysroot = SysRootKitAspect().sysRoot(runControl()->kit());
            if (sysroot.isEmpty())
                setSysRoot("/");
            else
                setSysRoot(sysroot);
        }

        DebuggerRunTool::start();
    }
};

// AppManagerQmlToolingSupport

class AppManagerQmlToolingSupport final : public RunWorker
{
public:
    explicit AppManagerQmlToolingSupport(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("AppManagerQmlToolingSupport");

        runControl->requestQmlChannel();
        QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());
        auto runner = createInferiorRunner(runControl, services);
        addStartDependency(runner);
        addStopDependency(runner);

        auto worker = runControl->createWorker(runnerIdForRunMode(runControl->runMode()));
        worker->addStartDependency(this);
        addStopDependency(worker);

        // Make sure the QML Profiler is stopped before the appman-controller
        runner->addStopDependency(worker);
    }
};

// AppManagerDevicePerfProfilerSupport

class AppManagerPerfProfilerSupport final : public RunWorker
{
public:
    explicit AppManagerPerfProfilerSupport(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("AppManagerPerfProfilerSupport");

        runControl->requestPerfChannel();
        auto profilee = createInferiorRunner(runControl, NoQmlDebugServices);
        addStartDependency(profilee);
        addStopDependency(profilee);
    }
};

// Factories

class AppManagerRunWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerRunWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            auto worker = new SimpleTargetRunner(runControl);
            worker->setId("ApplicationManagerPlugin.Run.TargetRunner");
            QObject::connect(worker, &RunWorker::stopped, worker, [worker, runControl] {
                worker->appendMessage(
                    Tr::tr("%1 exited.").arg(runControl->runnable().command.toUserOutput()),
                    OutputFormat::NormalMessageFormat);
            });

            worker->setStartModifier([worker, runControl] {
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
                worker->setEnvironment({});
                // Prevent the write channel to be closed, otherwise the appman-controller will exit
                worker->setProcessMode(ProcessMode::Writer);
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
                worker->setCommandLine(cmd);
            });
            return worker;
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
        setProduct<AppManagerDebugSupport>();
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::RUNANDDEBUGCONFIGURATION_ID);
    }
};

class AppManagerQmlToolingWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerQmlToolingWorkerFactory()
    {
        setProduct<AppManagerQmlToolingSupport>();
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
        setProduct<AppManagerPerfProfilerSupport>();
        addSupportedRunMode("PerfRecorder");
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
