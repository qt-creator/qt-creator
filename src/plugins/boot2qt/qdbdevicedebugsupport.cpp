// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevicedebugsupport.h"

#include "qdbconstants.h"

#include <perfprofiler/perfprofilerconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <debugger/debuggerruncontrol.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb::Internal {

static RunWorker *createQdbDeviceInferiorWorker(RunControl *runControl,
                                                QmlDebugServicesPreset qmlServices)
{
    auto worker = new SimpleTargetRunner(runControl);
    worker->setId("QdbDeviceInferiorWorker");

    worker->setStartModifier([worker, runControl, qmlServices] {
        CommandLine cmd{worker->device()->filePath(Constants::AppcontrollerFilepath)};

        int lowerPort = 0;
        int upperPort = 0;

        if (worker->usesDebugChannel()) {
            cmd.addArg("--debug-gdb");
            lowerPort = upperPort = worker->debugChannel().port();
        }
        if (worker->usesQmlChannel()) {
            cmd.addArg("--debug-qml");
            cmd.addArg("--qml-debug-services");
            cmd.addArg(qmlDebugServices(qmlServices));
            lowerPort = upperPort = worker->qmlChannel().port();
        }
        if (worker->usesDebugChannel() && worker->usesQmlChannel()) {
            lowerPort = worker->debugChannel().port();
            upperPort = worker->qmlChannel().port();
            if (lowerPort + 1 != upperPort) {
                worker->reportFailure("Need adjacent free ports for combined C++/QML debugging");
                return;
            }
        }
        if (worker->usesPerfChannel()) {
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
            lowerPort = upperPort = worker->perfChannel().port();
        }

        cmd.addArg("--port-range");
        cmd.addArg(QString("%1-%2").arg(lowerPort).arg(upperPort));
        cmd.addCommandLineAsArgs(runControl->commandLine());

        worker->setCommandLine(cmd);
        worker->setWorkingDirectory(runControl->workingDirectory());
        worker->setEnvironment(runControl->environment());
    });
    return worker;
}

// QdbDeviceDebugSupport

class QdbDeviceDebugSupport final : public Debugger::DebuggerRunTool
{
public:
    explicit QdbDeviceDebugSupport(RunControl *runControl);

private:
    void start() override;
};

QdbDeviceDebugSupport::QdbDeviceDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("QdbDeviceDebugSupport");

    if (isCppDebugging())
        runControl->requestDebugChannel();
    if (isQmlDebugging())
        runControl->requestQmlChannel();

    auto debuggee = createQdbDeviceInferiorWorker(runControl, QmlDebuggerServices);
    addStartDependency(debuggee);

    debuggee->addStopDependency(this);
}

void QdbDeviceDebugSupport::start()
{
    setStartMode(Debugger::AttachToRemoteServer);
    setCloseMode(KillAndExitMonitorAtClose);
    setUseContinueInsteadOfRun(true);
    setContinueAfterAttach(true);
    addSolibSearchDir("%{sysroot}/system/lib");
    DebuggerRunTool::start();
}

class QdbRunWorkerFactory final : public RunWorkerFactory
{
public:
    QdbRunWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            auto worker = new SimpleTargetRunner(runControl);
            worker->setStartModifier([worker] {
                const CommandLine remoteCommand = worker->commandLine();
                const FilePath remoteExe = remoteCommand.executable();
                CommandLine cmd{remoteExe.withNewPath(Constants::AppcontrollerFilepath)};
                cmd.addArg(remoteExe.nativePath());
                cmd.addArgs(remoteCommand.arguments(), CommandLine::Raw);
                worker->setCommandLine(cmd);
            });
            return worker;
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
        setProduct<QdbDeviceDebugSupport>();
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
            auto worker = new RunWorker(runControl);
            worker->setId("QdbDeviceQmlToolingSupport");

            runControl->requestQmlChannel();
            const QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());
            auto runner = createQdbDeviceInferiorWorker(runControl, services);
            worker->addStartDependency(runner);
            worker->addStopDependency(runner);

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
            auto worker = createQdbDeviceInferiorWorker(runControl, NoQmlDebugServices);
            return worker;
        });
        addSupportedRunMode("PerfRecorder");
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
