// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevicedebugsupport.h"

#include "qdbconstants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <debugger/debuggerruncontrol.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb::Internal {

class QdbDeviceInferiorRunner : public RunWorker
{
public:
    QdbDeviceInferiorRunner(RunControl *runControl,
                      bool usePerf, bool useGdbServer, bool useQmlServer,
                      QmlDebug::QmlDebugServicesPreset qmlServices)
        : RunWorker(runControl),
          m_usePerf(usePerf), m_useGdbServer(useGdbServer), m_useQmlServer(useQmlServer),
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

        m_portsGatherer = new DebugServerPortsGatherer(runControl);
        m_portsGatherer->setUseGdbServer(useGdbServer || usePerf);
        m_portsGatherer->setUseQmlServer(useQmlServer);
        addStartDependency(m_portsGatherer);
    }

    QUrl perfServer() const { return m_portsGatherer->gdbServer(); }
    QUrl gdbServer() const { return m_portsGatherer->gdbServer(); }
    QUrl qmlServer() const { return m_portsGatherer->qmlServer(); }

    void start() override
    {
        const int perfPort = m_portsGatherer->gdbServer().port();
        const int gdbServerPort = m_portsGatherer->gdbServer().port();
        const int qmlServerPort = m_portsGatherer->qmlServer().port();

        int lowerPort = 0;
        int upperPort = 0;

        CommandLine cmd;
        cmd.setExecutable(device()->filePath(Constants::AppcontrollerFilepath));

        if (m_useGdbServer) {
            cmd.addArg("--debug-gdb");
            lowerPort = upperPort = gdbServerPort;
        }
        if (m_useQmlServer) {
            cmd.addArg("--debug-qml");
            cmd.addArg("--qml-debug-services");
            cmd.addArg(QmlDebug::qmlDebugServices(m_qmlServices));
            lowerPort = upperPort = qmlServerPort;
        }
        if (m_useGdbServer && m_useQmlServer) {
            if (gdbServerPort + 1 != qmlServerPort) {
                reportFailure("Need adjacent free ports for combined C++/QML debugging");
                return;
            }
            lowerPort = gdbServerPort;
            upperPort = qmlServerPort;
        }
        if (m_usePerf) {
            Store settingsData = runControl()->settingsData("Analyzer.Perf.Settings");
            QVariant perfRecordArgs = settingsData.value("Analyzer.Perf.RecordArguments");
            QString args =  Utils::transform(perfRecordArgs.toStringList(), [](QString arg) {
                                    return arg.replace(',', ",,");
                            }).join(',');
            cmd.addArg("--profile-perf");
            cmd.addArg(args);
            lowerPort = upperPort = perfPort;
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
    Debugger::DebugServerPortsGatherer *m_portsGatherer = nullptr;
    bool m_usePerf;
    bool m_useGdbServer;
    bool m_useQmlServer;
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

    QdbDeviceInferiorRunner *m_debuggee = nullptr;
};

QdbDeviceDebugSupport::QdbDeviceDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("QdbDeviceDebugSupport");

    m_debuggee = new QdbDeviceInferiorRunner(runControl, false, isCppDebugging(), isQmlDebugging(),
                                             QmlDebug::QmlDebuggerServices);
    addStartDependency(m_debuggee);

    m_debuggee->addStopDependency(this);
}

void QdbDeviceDebugSupport::start()
{
    setStartMode(Debugger::AttachToRemoteServer);
    setCloseMode(KillAndExitMonitorAtClose);
    setRemoteChannel(m_debuggee->gdbServer());
    setQmlServer(m_debuggee->qmlServer());
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

private:
    void start() override;

    QdbDeviceInferiorRunner *m_runner = nullptr;
    RunWorker *m_worker = nullptr;
};

QdbDeviceQmlToolingSupport::QdbDeviceQmlToolingSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("QdbDeviceQmlToolingSupport");

    QmlDebug::QmlDebugServicesPreset services = QmlDebug::servicesForRunMode(runControl->runMode());
    m_runner = new QdbDeviceInferiorRunner(runControl, false, false, true, services);
    addStartDependency(m_runner);
    addStopDependency(m_runner);

    m_worker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
    m_worker->addStartDependency(this);
    addStopDependency(m_worker);
}

void QdbDeviceQmlToolingSupport::start()
{
    m_worker->recordData("QmlServerUrl", m_runner->qmlServer());
    reportStarted();
}

// QdbDevicePerfProfilerSupport

class QdbDevicePerfProfilerSupport final : public RunWorker
{
public:
    explicit QdbDevicePerfProfilerSupport(RunControl *runControl);

private:
    void start() override;

    QdbDeviceInferiorRunner *m_profilee = nullptr;
};

QdbDevicePerfProfilerSupport::QdbDevicePerfProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("QdbDevicePerfProfilerSupport");

    m_profilee = new QdbDeviceInferiorRunner(runControl, true, false, false,
                                             QmlDebug::NoQmlDebugServices);
    addStartDependency(m_profilee);
    addStopDependency(m_profilee);
}

void QdbDevicePerfProfilerSupport::start()
{
    runControl()->setProperty("PerfConnection", m_profilee->perfServer());
    reportStarted();
}

// Factories

QdbRunWorkerFactory::QdbRunWorkerFactory(const QList<Id> &runConfigs)
{
    setProduct<QdbDeviceRunSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
}

QdbDebugWorkerFactory::QdbDebugWorkerFactory(const QList<Id> &runConfigs)
{
    setProduct<QdbDeviceDebugSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
}

QdbQmlToolingWorkerFactory::QdbQmlToolingWorkerFactory(const QList<Id> &runConfigs)
{
    setProduct<QdbDeviceQmlToolingSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
}

QdbPerfProfilerWorkerFactory::QdbPerfProfilerWorkerFactory()
{
    setProduct<QdbDevicePerfProfilerSupport>();
    addSupportedRunMode("PerfRecorder");
    addSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
}

} // Qdb::Internal
