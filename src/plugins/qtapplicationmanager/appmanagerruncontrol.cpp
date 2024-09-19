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
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

// AppManagerRunner

class AppManagerRunner final : public SimpleTargetRunner
{
public:
    AppManagerRunner(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setId("ApplicationManagerPlugin.Run.TargetRunner");
        connect(this, &RunWorker::stopped, this, [this, runControl] {
            appendMessage(Tr::tr("%1 exited.").arg(runControl->runnable().command.toUserOutput()),
                          OutputFormat::NormalMessageFormat);
        });

        setStartModifier([this, runControl] {
            FilePath controller = runControl->aspectData<AppManagerControllerAspect>()->filePath;
            QString appId = runControl->aspectData<AppManagerIdAspect>()->value;
            QString instanceId = runControl->aspectData<AppManagerInstanceIdAspect>()->value;
            QString documentUrl = runControl->aspectData<AppManagerDocumentUrlAspect>()->value;
            bool restartIfRunning = runControl->aspectData<AppManagerRestartIfRunningAspect>()->value;
            QStringList envVars;
            if (auto envAspect = runControl->aspectData<EnvironmentAspect>())
                envVars = envAspect->environment.toStringList();

            // Always use the default environment to start the appman-controller in
            // The env variables from the EnvironmentAspect are set through the controller
            setEnvironment({});
            // Prevent the write channel to be closed, otherwise the appman-controller will exit
            setProcessMode(ProcessMode::Writer);
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
            setCommandLine(cmd);
        });
    }
};


// AppManInferiorRunner

class AppManInferiorRunner : public SimpleTargetRunner
{
public:
    AppManInferiorRunner(RunControl *runControl,
                         bool usePerf, bool useGdbServer, bool useQmlServer,
                         QmlDebug::QmlDebugServicesPreset qmlServices)
        : SimpleTargetRunner(runControl),
        m_usePerf(usePerf), m_useGdbServer(useGdbServer), m_useQmlServer(useQmlServer),
        m_qmlServices(qmlServices)
    {
        setId(AppManager::Constants::DEBUG_LAUNCHER_ID);
        setEssential(true);

        if (usePerf) {
            suppressDefaultStdOutHandling();
            runControl->setProperty("PerfProcess", QVariant::fromValue(process()));
        }

        m_portsGatherer = new Debugger::DebugServerPortsGatherer(runControl);
        m_portsGatherer->setUseGdbServer(useGdbServer || usePerf);
        m_portsGatherer->setUseQmlServer(useQmlServer);
        addStartDependency(m_portsGatherer);

        setStartModifier([this, runControl] {
            FilePath controller = runControl->aspectData<AppManagerControllerAspect>()->filePath;
            QString appId = runControl->aspectData<AppManagerIdAspect>()->value;
            QString instanceId = runControl->aspectData<AppManagerInstanceIdAspect>()->value;
            QString documentUrl = runControl->aspectData<AppManagerDocumentUrlAspect>()->value;
            bool restartIfRunning = runControl->aspectData<AppManagerRestartIfRunningAspect>()->value;
            QStringList envVars;
            if (auto envAspect = runControl->aspectData<EnvironmentAspect>())
                envVars = envAspect->environment.toStringList();

            const int gdbServerPort = m_portsGatherer->gdbServer().port();
            const int qmlServerPort = m_portsGatherer->qmlServer().port();

            CommandLine cmd{controller};
            if (!instanceId.isEmpty())
                cmd.addArgs({"--instance-id", instanceId});

            cmd.addArg("debug-application");

            if (m_useGdbServer || m_useQmlServer) {
                QStringList debugArgs;
                debugArgs.append(envVars.join(' '));
                if (m_useGdbServer) {
                    debugArgs.append(QString("gdbserver :%1").arg(gdbServerPort));
                }
                if (m_useQmlServer) {
                    debugArgs.append(QString("%program% %1 %arguments%")
                                         .arg(qmlDebugCommandLineArguments(m_qmlServices,
                                                                            QString("port:%1").arg(qmlServerPort),
                                                                            true)));
                }
                cmd.addArg(debugArgs.join(' '));
            }
            if (m_usePerf) {
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
            setEnvironment({});
            // Prevent the write channel to be closed, otherwise the appman-controller will exit
            setProcessMode(ProcessMode::Writer);
            setCommandLine(cmd);

            appendMessage(Tr::tr("Starting Application Manager debugging..."), NormalMessageFormat);
            appendMessage(Tr::tr("Using: %1.").arg(cmd.toUserOutput()), NormalMessageFormat);
        });
    }

    QUrl perfServer() const { return m_portsGatherer->gdbServer(); }
    QUrl gdbServer() const { return m_portsGatherer->gdbServer(); }
    QUrl qmlServer() const { return m_portsGatherer->qmlServer(); }

private:
    Debugger::DebugServerPortsGatherer *m_portsGatherer = nullptr;
    bool m_usePerf;
    bool m_useGdbServer;
    bool m_useQmlServer;
    QmlDebug::QmlDebugServicesPreset m_qmlServices;
};


// AppManagerDebugSupport

class AppManagerDebugSupport final : public Debugger::DebuggerRunTool
{
private:
    FilePath m_symbolFile;
    AppManInferiorRunner *m_debuggee = nullptr;

public:
    AppManagerDebugSupport(RunControl *runControl)
        : DebuggerRunTool(runControl)
    {
        setId("ApplicationManagerPlugin.Debug.Support");

        m_debuggee = new AppManInferiorRunner(runControl, false, isCppDebugging(), isQmlDebugging(),
                                              QmlDebug::QmlDebuggerServices);

        addStartDependency(m_debuggee);
        addStopDependency(m_debuggee);

        Target *target = runControl->target();

        const Internal::TargetInformation targetInformation(target);
        if (!targetInformation.isValid())
            return;

        if (targetInformation.manifest.isQmlRuntime()) {
            m_symbolFile = getToolFilePath(Constants::APPMAN_LAUNCHER_QML,
                                           target->kit(),
                                           DeviceKitAspect::device(target->kit()));
        } else if (targetInformation.manifest.isNativeRuntime()) {
            m_symbolFile = Utils::findOrDefault(target->buildSystem()->applicationTargets(), [&](const BuildTargetInfo &ti) {
                               return ti.buildKey == targetInformation.manifest.code || ti.projectFilePath.toString() == targetInformation.manifest.code;
                           }).targetFilePath;
        } else {
            reportFailure(Tr::tr("Cannot debug: Only QML and native applications are supported."));
        }
    }

private:
    void start() override
    {
        if (m_symbolFile.isEmpty()) {
            reportFailure(Tr::tr("Cannot debug: Local executable is not set."));
            return;
        }

        setStartMode(Debugger::AttachToRemoteServer);
        setCloseMode(Debugger::KillAndExitMonitorAtClose);

        if (isQmlDebugging())
            setQmlServer(m_debuggee->qmlServer());

        if (isCppDebugging()) {
            setUseExtendedRemote(false);
            setUseContinueInsteadOfRun(true);
            setContinueAfterAttach(true);
            setRemoteChannel(m_debuggee->gdbServer());
            setSymbolFile(m_symbolFile);

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

        QmlDebug::QmlDebugServicesPreset services = QmlDebug::servicesForRunMode(runControl->runMode());
        m_runner = new AppManInferiorRunner(runControl, false, false, true, services);
        addStartDependency(m_runner);
        addStopDependency(m_runner);

        m_worker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
        m_worker->addStartDependency(this);
        addStopDependency(m_worker);

        // Make sure the QML Profiler is stopped before the appman-controller
        m_runner->addStopDependency(m_worker);
    }

private:
    void start() final
    {
        m_worker->recordData("QmlServerUrl", m_runner->qmlServer());
        reportStarted();
    }

    AppManInferiorRunner *m_runner = nullptr;
    RunWorker *m_worker = nullptr;
};

// AppManagerDevicePerfProfilerSupport

class AppManagerPerfProfilerSupport final : public RunWorker
{
public:
    explicit AppManagerPerfProfilerSupport(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("AppManagerPerfProfilerSupport");

        m_profilee = new AppManInferiorRunner(runControl, true, false, false,
                                              QmlDebug::NoQmlDebugServices);
        addStartDependency(m_profilee);
        addStopDependency(m_profilee);
    }

private:
    AppManInferiorRunner *m_profilee = nullptr;
};

// Factories

class AppManagerRunWorkerFactory final : public RunWorkerFactory
{
public:
    AppManagerRunWorkerFactory()
    {
        setProduct<AppManagerRunner>();
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
