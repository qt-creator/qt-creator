/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbdevicedebugsupport.h"

#include "qdbconstants.h"
#include "qdbdevice.h"
#include "qdbrunconfiguration.h"

#include <debugger/debuggerruncontrol.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Qdb::Internal;

namespace Qdb {

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

        connect(&m_launcher, &QtcProcess::started, this, &RunWorker::reportStarted);
        connect(&m_launcher, &QtcProcess::finished, this, &RunWorker::reportStopped);

        connect(&m_launcher, &QtcProcess::readyReadStandardOutput, [this] {
                appendMessage(QString::fromUtf8(m_launcher.readAllStandardOutput()), StdOutFormat);
        });
        connect(&m_launcher, &QtcProcess::readyReadStandardError, [this] {
                appendMessage(QString::fromUtf8(m_launcher.readAllStandardError()), StdErrFormat);
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

        Runnable r = runnable();

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
            QVariantMap settingsData = runControl()->settingsData("Analyzer.Perf.Settings");
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
        cmd.addCommandLineAsArgs(r.command);

        m_launcher.setCommand(cmd);
        m_launcher.setWorkingDirectory(r.workingDirectory);
        m_launcher.setEnvironment(r.environment);
        m_launcher.start();
    }

    void stop() override { m_launcher.close(); }

private:
    Debugger::DebugServerPortsGatherer *m_portsGatherer = nullptr;
    bool m_usePerf;
    bool m_useGdbServer;
    bool m_useQmlServer;
    QmlDebug::QmlDebugServicesPreset m_qmlServices;
    QtcProcess m_launcher;
};


// QdbDeviceDebugSupport

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

} // namespace Qdb
