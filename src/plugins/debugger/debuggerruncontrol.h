/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>

namespace Debugger {

namespace Internal {
class TerminalRunner;
class DebuggerRunToolPrivate;
} // Internal

class GdbServerPortsGatherer;

class DEBUGGER_EXPORT DebuggerRunTool : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit DebuggerRunTool(ProjectExplorer::RunControl *runControl,
                             ProjectExplorer::Kit *kit = nullptr);
    ~DebuggerRunTool();

    Internal::DebuggerEngine *engine() const { return m_engine; }
    Internal::DebuggerEngine *activeEngine() const;

    void startRunControl();

    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1);

    void start() override;
    void stop() override;

    void notifyInferiorIll();
    Q_SLOT void notifyInferiorExited(); // Called from Android.
    void quitDebugger();
    void abortDebugger();

    const Internal::DebuggerRunParameters &runParameters() const;

    void startDying() { m_isDying = true; }
    bool isDying() const { return m_isDying; }
    bool isCppDebugging() const;
    bool isQmlDebugging() const;
    int portsUsedByDebugger() const;

    void setUsePortsGatherer(bool useCpp, bool useQml);
    GdbServerPortsGatherer *portsGatherer() const;

    void setSolibSearchPath(const QStringList &list);
    void addSolibSearchDir(const QString &str);

    static void setBreakOnMainNextTime();

    void setInferior(const ProjectExplorer::Runnable &runnable);
    void setInferiorExecutable(const QString &executable);
    void setInferiorEnvironment(const Utils::Environment &env); // Used by GammaRay plugin
    void setRunControlName(const QString &name);
    void setStartMessage(const QString &msg);
    void appendInferiorCommandLineArgument(const QString &arg);
    void prependInferiorCommandLineArgument(const QString &arg);
    void addQmlServerInferiorCommandLineArgumentIfNeeded();

    void setCrashParameter(const QString &event);

    void addExpectedSignal(const QString &signal);
    void addSearchDirectory(const QString &dir);

    void setStartMode(DebuggerStartMode startMode);
    void setCloseMode(DebuggerCloseMode closeMode);

    void setAttachPid(Utils::ProcessHandle pid);
    void setAttachPid(qint64 pid);

    void setSysRoot(const QString &sysRoot);
    void setSymbolFile(const QString &symbolFile);
    void setRemoteChannel(const QString &channel);
    void setRemoteChannel(const QString &host, int port);

    void setUseExtendedRemote(bool on);
    void setUseContinueInsteadOfRun(bool on);
    void setUseTargetAsync(bool on);
    void setContinueAfterAttach(bool on);
    void setSkipExecutableValidation(bool on);
    void setUseCtrlCStub(bool on);
    void setBreakOnMain(bool on);
    void setUseTerminal(bool on);

    void setCommandsAfterConnect(const QString &commands);
    void setCommandsForReset(const QString &commands);

    void setServerStartScript(const QString &serverStartScript);
    void setDebugInfoLocation(const QString &debugInfoLocation);

    void setQmlServer(const QUrl &qmlServer);

    void setCoreFileName(const QString &core, bool isSnapshot = false);

    void setIosPlatform(const QString &platform);
    void setDeviceSymbolsRoot(const QString &deviceSymbolsRoot);

    void setTestCase(int testCase);
    void setOverrideStartScript(const QString &script);

    Internal::TerminalRunner *terminalRunner() const;

signals:
    void aboutToNotifyInferiorSetupOk();

private:
    bool fixupParameters();

    Internal::DebuggerRunToolPrivate *d;
    QPointer<Internal::DebuggerEngine> m_engine; // Master engine
    Internal::DebuggerRunParameters m_runParameters;
    bool m_isDying = false;
};

class DEBUGGER_EXPORT GdbServerPortsGatherer : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit GdbServerPortsGatherer(ProjectExplorer::RunControl *runControl);
    ~GdbServerPortsGatherer();

    void setUseGdbServer(bool useIt) { m_useGdbServer = useIt; }
    bool useGdbServer() const { return m_useGdbServer; }
    Utils::Port gdbServerPort() const { return m_gdbServerPort; }
    QString gdbServerChannel() const;

    void setUseQmlServer(bool useIt) { m_useQmlServer = useIt; }
    bool useQmlServer() const { return m_useQmlServer; }
    Utils::Port qmlServerPort() const { return m_qmlServerPort; }
    QUrl qmlServer() const;

    void setDevice(ProjectExplorer::IDevice::ConstPtr device);

private:
    void start() override;
    void handlePortListReady();

    ProjectExplorer::DeviceUsedPortsGatherer m_portsGatherer;
    bool m_useGdbServer = false;
    bool m_useQmlServer = false;
    Utils::Port m_gdbServerPort;
    Utils::Port m_qmlServerPort;
    ProjectExplorer::IDevice::ConstPtr m_device;
};

class DEBUGGER_EXPORT GdbServerRunner : public ProjectExplorer::SimpleTargetRunner
{
    Q_OBJECT

public:
    explicit GdbServerRunner(ProjectExplorer::RunControl *runControl,
                             GdbServerPortsGatherer *portsGatherer);

    ~GdbServerRunner();

    void setRunnable(const ProjectExplorer::StandardRunnable &runnable);
    void setUseMulti(bool on);
    void setAttachPid(Utils::ProcessHandle pid);

private:
    void start() override;

    GdbServerPortsGatherer *m_portsGatherer;
    ProjectExplorer::StandardRunnable m_runnable;
    Utils::ProcessHandle m_pid;
    bool m_useMulti = true;
};

extern DEBUGGER_EXPORT const char GdbServerRunnerWorkerId[];
extern DEBUGGER_EXPORT const char GdbServerPortGathererWorkerId[];

} // namespace Debugger
