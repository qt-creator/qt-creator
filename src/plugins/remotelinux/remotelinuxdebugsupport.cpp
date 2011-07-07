/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "remotelinuxdebugsupport.h"

#include "maemoglobal.h"
#include "maemousedportsgatherer.h"
#include "qt4maemotarget.h"
#include "remotelinuxapplicationrunner.h"

#include <debugger/debuggerengine.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Utils;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
using namespace Internal;

DebuggerStartParameters AbstractRemoteLinuxDebugSupport::startParameters(const RemoteLinuxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    const LinuxDeviceConfiguration::ConstPtr &devConf = runConfig->deviceConfig();
    const RemoteLinuxRunConfiguration::DebuggingType debuggingType
        = runConfig->debuggingType();
    if (debuggingType != RemoteLinuxRunConfiguration::DebugCppOnly) {
        params.qmlServerAddress = runConfig->deviceConfig()->sshParameters().host;
        params.qmlServerPort = -1;
    }
    if (debuggingType != RemoteLinuxRunConfiguration::DebugQmlOnly) {
        params.processArgs = runConfig->arguments();
        if (runConfig->activeQt4BuildConfiguration()->qtVersion())
            params.sysroot = runConfig->activeQt4BuildConfiguration()->qtVersion()->systemRoot();
        params.toolChainAbi = runConfig->abi();
        params.startMode = AttachToRemote;
        params.executable = runConfig->localExecutableFilePath();
        params.debuggerCommand = runConfig->gdbCmd();
        params.remoteChannel = devConf->sshParameters().host + QLatin1String(":-1");
        params.useServerStartScript = true;

        // TODO: This functionality should be inside the debugger.
        const ProjectExplorer::Abi &abi = runConfig->target()
            ->activeBuildConfiguration()->toolChain()->targetAbi();
        params.remoteArchitecture = abi.toString();
        params.gnuTarget = QLatin1String(abi.architecture() == ProjectExplorer::Abi::ArmArchitecture
            ? "arm-none-linux-gnueabi": "i386-unknown-linux-gnu");
    } else {
        params.startMode = AttachToRemote;
    }
    params.displayName = runConfig->displayName();

    if (const ProjectExplorer::Project *project = runConfig->target()->project()) {
        params.projectSourceDirectory = project->projectDirectory();
        if (const ProjectExplorer::BuildConfiguration *buildConfig = runConfig->target()->activeBuildConfiguration()) {
            params.projectBuildDirectory = buildConfig->buildDirectory();
        }
        params.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
    }

    return params;
}

AbstractRemoteLinuxDebugSupport::AbstractRemoteLinuxDebugSupport(RemoteLinuxRunConfiguration *runConfig, DebuggerEngine *engine)
    : QObject(engine), m_engine(engine), m_runConfig(runConfig),
      m_deviceConfig(m_runConfig->deviceConfig()),
      m_debuggingType(runConfig->debuggingType()),
      m_state(Inactive), m_gdbServerPort(-1), m_qmlPort(-1)
{
    connect(m_engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleAdapterSetupRequested()));
}

AbstractRemoteLinuxDebugSupport::~AbstractRemoteLinuxDebugSupport()
{
    setState(Inactive);
}

void AbstractRemoteLinuxDebugSupport::showMessage(const QString &msg, int channel)
{
    if (m_engine)
        m_engine->showMessage(msg, channel);
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupRequested()
{
    ASSERT_STATE(Inactive);

    setState(StartingRunner);
    showMessage(tr("Preparing remote side ...\n"), AppStuff);
    disconnect(runner(), 0, this, 0);
    connect(runner(), SIGNAL(error(QString)), this, SLOT(handleSshError(QString)));
    connect(runner(), SIGNAL(readyForExecution()), this, SLOT(startExecution()));
    connect(runner(), SIGNAL(reportProgress(QString)), this, SLOT(handleProgressReport(QString)));
    runner()->start();
}

void AbstractRemoteLinuxDebugSupport::handleSshError(const QString &error)
{
    if (m_state == Debugging) {
        showMessage(error, AppError);
        if (m_engine)
            m_engine->notifyInferiorIll();
    } else if (m_state != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void AbstractRemoteLinuxDebugSupport::startExecution()
{
    if (m_state == Inactive)
        return;

    ASSERT_STATE(StartingRunner);

    if (m_debuggingType != RemoteLinuxRunConfiguration::DebugQmlOnly) {
        if (!setPort(m_gdbServerPort))
            return;
    }
    if (m_debuggingType != RemoteLinuxRunConfiguration::DebugCppOnly) {
        if (!setPort(m_qmlPort))
            return;
    }

    setState(StartingRemoteProcess);
    m_gdbserverOutput.clear();
    connect(runner(), SIGNAL(remoteErrorOutput(QByteArray)), this,
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner(), SIGNAL(remoteOutput(QByteArray)), this,
        SLOT(handleRemoteOutput(QByteArray)));
    if (m_debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly) {
        connect(runner(), SIGNAL(remoteProcessStarted()),
            SLOT(handleRemoteProcessStarted()));
    }
    const QString &remoteExe = runner()->remoteExecutable();
    QString args = runner()->arguments();
    if (m_debuggingType != RemoteLinuxRunConfiguration::DebugCppOnly) {
        args += QString(QLatin1String(" -qmljsdebugger=port:%1,block"))
            .arg(m_qmlPort);
    }

    const QString remoteCommandLine = m_debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly
        ? QString::fromLocal8Bit("%1 %2 %3").arg(runner()->commandPrefix()).arg(remoteExe).arg(args)
        : QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4").arg(runner()->commandPrefix())
              .arg(m_gdbServerPort).arg(remoteExe).arg(args);
    connect(runner(), SIGNAL(remoteProcessFinished(qint64)),
        SLOT(handleRemoteProcessFinished(qint64)));
    runner()->startExecution(remoteCommandLine.toUtf8());
}

void AbstractRemoteLinuxDebugSupport::handleRemoteProcessFinished(qint64 exitCode)
{
    if (!m_engine || m_state == Inactive)
        return;

    if (m_state == Debugging) {
        // The QML engine does not realize on its own that the application has finished.
        if (m_debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly)
            m_engine->quitDebugger();
        else if (exitCode != 0)
            m_engine->notifyInferiorIll();

    } else {
        const QString errorMsg = m_debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly
            ? tr("Remote application failed with exit code %1.").arg(exitCode)
            : tr("The gdbserver process closed unexpectedly.");
        m_engine->handleRemoteSetupFailed(errorMsg);
    }
}

void AbstractRemoteLinuxDebugSupport::handleDebuggingFinished()
{
    setState(Inactive);
}

void AbstractRemoteLinuxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << Inactive << Debugging);
    showMessage(QString::fromUtf8(output), AppOutput);
}

void AbstractRemoteLinuxDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << Inactive << StartingRemoteProcess << Debugging);

    if (!m_engine)
        return;

    showMessage(QString::fromUtf8(output), AppOutput);
    if (m_state == StartingRemoteProcess
            && m_debuggingType != RemoteLinuxRunConfiguration::DebugQmlOnly) {
        m_gdbserverOutput += output;
        if (m_gdbserverOutput.contains("Listening on port")) {
            handleAdapterSetupDone();
            m_gdbserverOutput.clear();
        }
    }
}

void AbstractRemoteLinuxDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), AppStuff);
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    setState(Inactive);
    m_engine->handleRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupDone()
{
    setState(Debugging);
    m_engine->handleRemoteSetupDone(m_gdbServerPort, m_qmlPort);
}

void AbstractRemoteLinuxDebugSupport::handleRemoteProcessStarted()
{
    Q_ASSERT(m_debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly);
    ASSERT_STATE(StartingRemoteProcess);
    handleAdapterSetupDone();
}

void AbstractRemoteLinuxDebugSupport::setState(State newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    if (m_state == Inactive)
        runner()->stop();
}

bool AbstractRemoteLinuxDebugSupport::setPort(int &port)
{
    port = runner()->usedPortsGatherer()->getNextFreePort(runner()->freePorts());
    if (port == -1) {
        handleAdapterSetupFailed(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}


RemoteLinuxDebugSupport::RemoteLinuxDebugSupport(RemoteLinuxRunConfiguration *runConfig,
        DebuggerEngine *engine)
    : AbstractRemoteLinuxDebugSupport(runConfig, engine),
      m_runner(new RemoteLinuxApplicationRunner(this, runConfig))
{
}

RemoteLinuxDebugSupport::~RemoteLinuxDebugSupport()
{
}

} // namespace RemoteLinux
