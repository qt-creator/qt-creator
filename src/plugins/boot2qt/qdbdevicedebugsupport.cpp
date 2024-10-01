// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevicedebugsupport.h"

#include "qdbconstants.h"

#include <perfprofiler/perfprofilerconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <debugger/debuggerruncontrol.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb::Internal {

class QdbDeviceInferiorRunner : public RunWorker
{
public:
    QdbDeviceInferiorRunner(RunControl *runControl, QmlDebug::QmlDebugServicesPreset qmlServices)
        : RunWorker(runControl),
          m_qmlServices(qmlServices)
    {
        setId("QdbDebuggeeRunner");

        connect(&m_launcher, &Process::started, this, &RunWorker::reportStarted);
        connect(&m_launcher, &Process::done, this, &RunWorker::reportStopped);

        connect(&m_launcher, &Process::readyReadStandardOutput, this, [this] {
                appendMessage(m_launcher.readAllStandardOutput(), StdOutFormat);
        });
        connect(&m_launcher, &Process::readyReadStandardError, this, [this] {
                appendMessage(m_launcher.readAllStandardError(), StdErrFormat);
        });
    }

    void start() override
    {
        int lowerPort = 0;
        int upperPort = 0;

        CommandLine cmd;
        cmd.setExecutable(device()->filePath(Constants::AppcontrollerFilepath));

        if (usesDebugChannel()) {
            cmd.addArg("--debug-gdb");
            lowerPort = upperPort = debugChannel().port();
        }
        if (usesQmlChannel()) {
            cmd.addArg("--debug-qml");
            cmd.addArg("--qml-debug-services");
            cmd.addArg(QmlDebug::qmlDebugServices(m_qmlServices));
            lowerPort = upperPort = qmlChannel().port();
        }
        if (usesDebugChannel() && usesQmlChannel()) {
            lowerPort = debugChannel().port();
            upperPort = qmlChannel().port();
            if (lowerPort + 1 != upperPort) {
                reportFailure("Need adjacent free ports for combined C++/QML debugging");
                return;
            }
        }
        if (usesPerfChannel()) {
            const Store perfArgs = runControl()->settingsData(PerfProfiler::Constants::PerfSettingsId);
            const QString recordArgs = perfArgs[PerfProfiler::Constants::PerfRecordArgsId].toString();
            cmd.addArg("--profile-perf");
            cmd.addArgs(recordArgs, CommandLine::Raw);
            lowerPort = upperPort = perfChannel().port();
        }
        cmd.addArg("--port-range");
        cmd.addArg(QString("%1-%2").arg(lowerPort).arg(upperPort));
        cmd.addCommandLineAsArgs(runControl()->commandLine());

        m_launcher.setCommand(cmd);
        m_launcher.setWorkingDirectory(runControl()->workingDirectory());
        m_launcher.setEnvironment(runControl()->environment());
        m_launcher.start();
    }

    void stop() override { m_launcher.close(); }

private:
    friend class QdbDeviceDebugSupport;
    friend class QdbDeviceQmlProfilerSupport;
    friend class QdbDeviceQmlToolingSupport;
    friend class QdbDevicePerfProfilerSupport;

    QmlDebug::QmlDebugServicesPreset m_qmlServices;
    Process m_launcher;
};

// QdbDeviceRunSupport

class QdbDeviceRunSupport : public SimpleTargetRunner
{
public:
    QdbDeviceRunSupport(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setStartModifier([this] {
            const CommandLine remoteCommand = commandLine();
            const FilePath remoteExe = remoteCommand.executable();
            CommandLine cmd{remoteExe.withNewPath(Constants::AppcontrollerFilepath)};
            cmd.addArg(remoteExe.nativePath());
            cmd.addArgs(remoteCommand.arguments(), CommandLine::Raw);
            setCommandLine(cmd);
        });
    }
};


// QdbDeviceDebugSupport

class QdbDeviceDebugSupport final : public Debugger::DebuggerRunTool
{
public:
    explicit QdbDeviceDebugSupport(RunControl *runControl);

private:
    void start() override;
    void stop() override;
};

QdbDeviceDebugSupport::QdbDeviceDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("QdbDeviceDebugSupport");

    if (isCppDebugging())
        runControl->requestDebugChannel();
    if (isQmlDebugging())
        runControl->requestQmlChannel();

    auto debuggee = new QdbDeviceInferiorRunner(runControl, QmlDebug::QmlDebuggerServices);
    addStartDependency(debuggee);

    debuggee->addStopDependency(this);
}

void QdbDeviceDebugSupport::start()
{
    setStartMode(Debugger::AttachToRemoteServer);
    setCloseMode(KillAndExitMonitorAtClose);
    if (usesDebugChannel())
        setRemoteChannel(debugChannel());
    if (usesQmlChannel())
        setQmlServer(qmlChannel());
    setUseContinueInsteadOfRun(true);
    setContinueAfterAttach(true);
    addSolibSearchDir("%{sysroot}/system/lib");

    DebuggerRunTool::start();
}

void QdbDeviceDebugSupport::stop()
{
    // Do nothing unusual. The launcher will die as result of (gdb) kill.
    DebuggerRunTool::stop();
}


// QdbDeviceQmlProfilerSupport

class QdbDeviceQmlToolingSupport final : public RunWorker
{
public:
    explicit QdbDeviceQmlToolingSupport(RunControl *runControl);
};

QdbDeviceQmlToolingSupport::QdbDeviceQmlToolingSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("QdbDeviceQmlToolingSupport");

    runControl->requestQmlChannel();
    QmlDebug::QmlDebugServicesPreset services = QmlDebug::servicesForRunMode(runControl->runMode());
    auto runner = new QdbDeviceInferiorRunner(runControl, services);
    addStartDependency(runner);
    addStopDependency(runner);

    auto worker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
    worker->addStartDependency(this);
    addStopDependency(worker);
}

// QdbDevicePerfProfilerSupport

class QdbDevicePerfProfilerSupport final : public RunWorker
{
public:
    explicit QdbDevicePerfProfilerSupport(RunControl *runControl);

private:
    void start() override;
};

QdbDevicePerfProfilerSupport::QdbDevicePerfProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("QdbDevicePerfProfilerSupport");

    runControl->requestPerfChannel();
    auto profilee = new QdbDeviceInferiorRunner(runControl, QmlDebug::NoQmlDebugServices);
    addStartDependency(profilee);
    addStopDependency(profilee);
}

void QdbDevicePerfProfilerSupport::start()
{
    QTC_ASSERT(usesPerfChannel(), reportFailure({}));
    runControl()->setProperty("PerfConnection", perfChannel());
    reportStarted();
}

// Factories

class QdbRunWorkerFactory final : public RunWorkerFactory
{
public:
    QdbRunWorkerFactory()
    {
        setProduct<QdbDeviceRunSupport>();
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
        setProduct<QdbDeviceQmlToolingSupport>();
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
        setProduct<QdbDevicePerfProfilerSupport>();
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
