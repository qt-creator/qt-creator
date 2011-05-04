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

#include "maemodebugsupport.h"

#include "maemodeployables.h"
#include "maemoglobal.h"
#include "maemosshrunner.h"
#include "maemousedportsgatherer.h"
#include "qt4maemotarget.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerengine.h>
#include <projectexplorer/abi.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Utils;
using namespace Debugger;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

RunControl *MaemoDebugSupport::createDebugRunControl(MaemoRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    const MaemoDeviceConfig::ConstPtr &devConf = runConfig->deviceConfig();

    const MaemoRunConfiguration::DebuggingType debuggingType
        = runConfig->debuggingType();
    if (debuggingType != MaemoRunConfiguration::DebugCppOnly) {
        params.qmlServerAddress = runConfig->deviceConfig()->sshParameters().host;
        params.qmlServerPort = -1;
    }
    if (debuggingType != MaemoRunConfiguration::DebugQmlOnly) {
        params.processArgs = runConfig->arguments();
        if (runConfig->activeQt4BuildConfiguration()->qtVersion())
            params.sysRoot = runConfig->activeQt4BuildConfiguration()->qtVersion()->systemRoot();
        params.toolChainAbi = runConfig->abi();
        if (runConfig->useRemoteGdb()) {
            params.startMode = StartRemoteGdb;
            params.executable = runConfig->remoteExecutableFilePath();
            params.debuggerCommand = MaemoGlobal::remoteCommandPrefix(runConfig->deviceConfig()->osVersion(),
                runConfig->deviceConfig()->sshParameters().userName,
                runConfig->remoteExecutableFilePath())
                + MaemoGlobal::remoteEnvironment(runConfig->userEnvironmentChanges())
                + QLatin1String(" /usr/bin/gdb");
            params.connParams = devConf->sshParameters();
            params.localMountDir = runConfig->localDirToMountForRemoteGdb();
            params.remoteMountPoint
                = runConfig->remoteProjectSourcesMountPoint();
            const QString execDirAbs
                = QDir::fromNativeSeparators(QFileInfo(runConfig->localExecutableFilePath()).path());
            const QString execDirRel
                = QDir(params.localMountDir).relativeFilePath(execDirAbs);
            params.remoteSourcesDir = QString(params.remoteMountPoint
                + QLatin1Char('/') + execDirRel).toUtf8();
        } else {
            params.startMode = AttachToRemote;
            params.executable = runConfig->localExecutableFilePath();
            params.debuggerCommand = runConfig->gdbCmd();
            params.remoteChannel
                = devConf->sshParameters().host + QLatin1String(":-1");
            params.useServerStartScript = true;
            const AbstractQt4MaemoTarget::DebugArchitecture &debugArch
                = runConfig->maemoTarget()->debugArchitecture();
            params.remoteArchitecture = debugArch.architecture;
            params.gnuTarget = debugArch.gnuTarget;
        }
    } else {
        params.startMode = AttachToRemote;
    }
    params.displayName = runConfig->displayName();

    DebuggerRunControl * const runControl =
        DebuggerPlugin::createDebugger(params, runConfig);
    bool useGdb = params.startMode == StartRemoteGdb
        && debuggingType != MaemoRunConfiguration::DebugQmlOnly;
    MaemoDebugSupport *debugSupport =
        new MaemoDebugSupport(runConfig, runControl->engine(), useGdb);
    connect(runControl, SIGNAL(finished()),
        debugSupport, SLOT(handleDebuggingFinished()));
    return runControl;
}

MaemoDebugSupport::MaemoDebugSupport(MaemoRunConfiguration *runConfig,
    DebuggerEngine *engine, bool useGdb)
    : QObject(engine), m_engine(engine), m_runConfig(runConfig),
      m_deviceConfig(m_runConfig->deviceConfig()),
      m_runner(new MaemoSshRunner(this, runConfig, true)),
      m_debuggingType(runConfig->debuggingType()),
      m_userEnvChanges(runConfig->userEnvironmentChanges()),
      m_state(Inactive), m_gdbServerPort(-1), m_qmlPort(-1),
      m_useGdb(useGdb)
{
    connect(m_engine, SIGNAL(requestRemoteSetup()), this,
        SLOT(handleAdapterSetupRequested()));
}

MaemoDebugSupport::~MaemoDebugSupport()
{
    setState(Inactive);
}

void MaemoDebugSupport::showMessage(const QString &msg, int channel)
{
    if (m_engine)
        m_engine->showMessage(msg, channel);
}

void MaemoDebugSupport::handleAdapterSetupRequested()
{
    ASSERT_STATE(Inactive);

    setState(StartingRunner);
    showMessage(tr("Preparing remote side ...\n"), AppStuff);
    disconnect(m_runner, 0, this, 0);
    connect(m_runner, SIGNAL(error(QString)), this,
        SLOT(handleSshError(QString)));
    connect(m_runner, SIGNAL(readyForExecution()), this,
        SLOT(startExecution()));
    connect(m_runner, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    m_runner->start();
}

void MaemoDebugSupport::handleSshError(const QString &error)
{
    if (m_state == Debugging) {
        showMessage(error, AppError);
        if (m_engine)
            m_engine->notifyInferiorIll();
    } else if (m_state != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void MaemoDebugSupport::startExecution()
{
    if (m_state == Inactive)
        return;

    ASSERT_STATE(StartingRunner);

    if (!useGdb() && m_debuggingType != MaemoRunConfiguration::DebugQmlOnly) {
        if (!setPort(m_gdbServerPort))
            return;
    }
    if (m_debuggingType != MaemoRunConfiguration::DebugCppOnly) {
        if (!setPort(m_qmlPort))
            return;
    }

    if (useGdb()) {
        handleAdapterSetupDone();
        return;
    }

    setState(StartingRemoteProcess);
    m_gdbserverOutput.clear();
    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)), this,
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)), this,
        SLOT(handleRemoteOutput(QByteArray)));
    if (m_debuggingType == MaemoRunConfiguration::DebugQmlOnly) {
        connect(m_runner, SIGNAL(remoteProcessStarted()),
            SLOT(handleRemoteProcessStarted()));
    }
    const QString &remoteExe = m_runner->remoteExecutable();
    const QString cmdPrefix = MaemoGlobal::remoteCommandPrefix(m_deviceConfig->osVersion(),
        m_deviceConfig->sshParameters().userName, remoteExe);
    const QString env = MaemoGlobal::remoteEnvironment(m_userEnvChanges);
    QString args = m_runner->arguments();
    if (m_debuggingType != MaemoRunConfiguration::DebugCppOnly) {
        args += QString(QLatin1String(" -qmljsdebugger=port:%1,block"))
            .arg(m_qmlPort);
    }

    const QString remoteCommandLine = m_debuggingType == MaemoRunConfiguration::DebugQmlOnly
        ? QString::fromLocal8Bit("%1 %2 %3 %4").arg(cmdPrefix).arg(env)
            .arg(remoteExe).arg(args)
        : QString::fromLocal8Bit("%1 %2 gdbserver :%3 %4 %5")
            .arg(cmdPrefix).arg(env).arg(m_gdbServerPort)
            .arg(remoteExe).arg(args);
    connect(m_runner, SIGNAL(remoteProcessFinished(qint64)),
        SLOT(handleRemoteProcessFinished(qint64)));
    m_runner->startExecution(remoteCommandLine.toUtf8());
}

void MaemoDebugSupport::handleRemoteProcessFinished(qint64 exitCode)
{
    if (!m_engine || m_state == Inactive || exitCode == 0)
        return;

    if (m_state == Debugging) {
        if (m_debuggingType != MaemoRunConfiguration::DebugQmlOnly)
            m_engine->notifyInferiorIll();
    } else {
        const QString errorMsg = m_debuggingType == MaemoRunConfiguration::DebugQmlOnly
            ? tr("Remote application failed with exit code %1.").arg(exitCode)
            : tr("The gdbserver process closed unexpectedly.");
        m_engine->handleRemoteSetupFailed(errorMsg);
    }
}

void MaemoDebugSupport::handleDebuggingFinished()
{
    setState(Inactive);
}

void MaemoDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << Inactive << Debugging);
    showMessage(QString::fromUtf8(output), AppOutput);
}

void MaemoDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << Inactive << StartingRemoteProcess << Debugging);

    if (!m_engine)
        return;

    showMessage(QString::fromUtf8(output), AppOutput);
    if (m_state == StartingRemoteProcess
            && m_debuggingType != MaemoRunConfiguration::DebugQmlOnly) {
        m_gdbserverOutput += output;
        if (m_gdbserverOutput.contains("Listening on port")) {
            handleAdapterSetupDone();
            m_gdbserverOutput.clear();
        }
    }
}

void MaemoDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), AppStuff);
}

void MaemoDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    setState(Inactive);
    m_engine->handleRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void MaemoDebugSupport::handleAdapterSetupDone()
{
    setState(Debugging);
    m_engine->handleRemoteSetupDone(m_gdbServerPort, m_qmlPort);
}

void MaemoDebugSupport::handleRemoteProcessStarted()
{
    Q_ASSERT(m_debuggingType == MaemoRunConfiguration::DebugQmlOnly);
    ASSERT_STATE(StartingRemoteProcess);
    handleAdapterSetupDone();
}

void MaemoDebugSupport::setState(State newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    if (m_state == Inactive)
        m_runner->stop();
}

bool MaemoDebugSupport::useGdb() const
{
    return m_useGdb;
}

bool MaemoDebugSupport::setPort(int &port)
{
    port = m_runner->usedPortsGatherer()->getNextFreePort(m_runner->freePorts());
    if (port == -1) {
        handleAdapterSetupFailed(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
