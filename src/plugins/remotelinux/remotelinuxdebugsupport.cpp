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

#include "linuxdeviceconfiguration.h"
#include "remotelinuxapplicationrunner.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxusedportsgatherer.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

using namespace Utils;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, StartingRunner, StartingRemoteProcess, Debugging };
} // anonymous namespace

class AbstractRemoteLinuxDebugSupportPrivate
{
public:
    AbstractRemoteLinuxDebugSupportPrivate(RemoteLinuxRunConfiguration *runConfig,
            DebuggerEngine *engine)
        : engine(engine), runConfig(runConfig), deviceConfig(runConfig->deviceConfig()),
          debuggingType(runConfig->debuggingType()), state(Inactive),
          gdbServerPort(-1), qmlPort(-1)
    {
    }

    const QPointer<Debugger::DebuggerEngine> engine;
    const QPointer<RemoteLinuxRunConfiguration> runConfig;
    const LinuxDeviceConfiguration::ConstPtr deviceConfig;
    const RemoteLinuxRunConfiguration::DebuggingType debuggingType;

    QByteArray gdbserverOutput;
    State state;
    int gdbServerPort;
    int qmlPort;
};

class RemoteLinuxDebugSupportPrivate
{
public:
    RemoteLinuxDebugSupportPrivate(RemoteLinuxRunConfiguration *runConfig) : runner(runConfig) {}

    GenericRemoteLinuxApplicationRunner runner;
};

} // namespace Internal

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

AbstractRemoteLinuxDebugSupport::AbstractRemoteLinuxDebugSupport(RemoteLinuxRunConfiguration *runConfig,
        DebuggerEngine *engine)
    : QObject(engine), m_d(new AbstractRemoteLinuxDebugSupportPrivate(runConfig, engine))
{
    connect(m_d->engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleAdapterSetupRequested()));
}

AbstractRemoteLinuxDebugSupport::~AbstractRemoteLinuxDebugSupport()
{
    setFinished();
    delete m_d;
}

void AbstractRemoteLinuxDebugSupport::showMessage(const QString &msg, int channel)
{
    if (m_d->engine)
        m_d->engine->showMessage(msg, channel);
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(m_d->state == Inactive, return);

    m_d->state = StartingRunner;
    showMessage(tr("Preparing remote side ...\n"), AppStuff);
    disconnect(runner(), 0, this, 0);
    connect(runner(), SIGNAL(error(QString)), this, SLOT(handleSshError(QString)));
    connect(runner(), SIGNAL(readyForExecution()), this, SLOT(startExecution()));
    connect(runner(), SIGNAL(reportProgress(QString)), this, SLOT(handleProgressReport(QString)));
    runner()->start();
}

void AbstractRemoteLinuxDebugSupport::handleSshError(const QString &error)
{
    if (m_d->state == Debugging) {
        showMessage(error, AppError);
        if (m_d->engine)
            m_d->engine->notifyInferiorIll();
    } else if (m_d->state != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void AbstractRemoteLinuxDebugSupport::startExecution()
{
    if (m_d->state == Inactive)
        return;

    QTC_ASSERT(m_d->state == StartingRunner, return);

    if (m_d->debuggingType != RemoteLinuxRunConfiguration::DebugQmlOnly) {
        if (!setPort(m_d->gdbServerPort))
            return;
    }
    if (m_d->debuggingType != RemoteLinuxRunConfiguration::DebugCppOnly) {
        if (!setPort(m_d->qmlPort))
            return;
    }

    m_d->state = StartingRemoteProcess;
    m_d->gdbserverOutput.clear();
    connect(runner(), SIGNAL(remoteErrorOutput(QByteArray)), this,
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner(), SIGNAL(remoteOutput(QByteArray)), this,
        SLOT(handleRemoteOutput(QByteArray)));
    if (m_d->debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly) {
        connect(runner(), SIGNAL(remoteProcessStarted()),
            SLOT(handleRemoteProcessStarted()));
    }
    const QString &remoteExe = runner()->remoteExecutable();
    QString args = runner()->arguments();
    if (m_d->debuggingType != RemoteLinuxRunConfiguration::DebugCppOnly) {
        args += QString(QLatin1String(" -qmljsdebugger=port:%1,block"))
            .arg(m_d->qmlPort);
    }

    const QString remoteCommandLine = m_d->debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly
        ? QString::fromLocal8Bit("%1 %2 %3").arg(runner()->commandPrefix()).arg(remoteExe).arg(args)
        : QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4").arg(runner()->commandPrefix())
              .arg(m_d->gdbServerPort).arg(remoteExe).arg(args);
    connect(runner(), SIGNAL(remoteProcessFinished(qint64)),
        SLOT(handleRemoteProcessFinished(qint64)));
    runner()->startExecution(remoteCommandLine.toUtf8());
}

void AbstractRemoteLinuxDebugSupport::handleRemoteProcessFinished(qint64 exitCode)
{
    if (!m_d->engine || m_d->state == Inactive)
        return;

    if (m_d->state == Debugging) {
        // The QML engine does not realize on its own that the application has finished.
        if (m_d->debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly)
            m_d->engine->quitDebugger();
        else if (exitCode != 0)
            m_d->engine->notifyInferiorIll();

    } else {
        const QString errorMsg = m_d->debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly
            ? tr("Remote application failed with exit code %1.").arg(exitCode)
            : tr("The gdbserver process closed unexpectedly.");
        m_d->engine->handleRemoteSetupFailed(errorMsg);
    }
}

void AbstractRemoteLinuxDebugSupport::handleDebuggingFinished()
{
    setFinished();
}

void AbstractRemoteLinuxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(m_d->state == Inactive || m_d->state == Debugging, return);

    showMessage(QString::fromUtf8(output), AppOutput);
}

void AbstractRemoteLinuxDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(m_d->state == Inactive || m_d->state == StartingRemoteProcess || m_d->state == Debugging,
        return);

    if (!m_d->engine)
        return;

    showMessage(QString::fromUtf8(output), AppOutput);
    if (m_d->state == StartingRemoteProcess
            && m_d->debuggingType != RemoteLinuxRunConfiguration::DebugQmlOnly) {
        m_d->gdbserverOutput += output;
        if (m_d->gdbserverOutput.contains("Listening on port")) {
            handleAdapterSetupDone();
            m_d->gdbserverOutput.clear();
        }
    }
}

void AbstractRemoteLinuxDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), AppStuff);
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    setFinished();
    m_d->engine->handleRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void AbstractRemoteLinuxDebugSupport::handleAdapterSetupDone()
{
    m_d->state = Debugging;
    m_d->engine->handleRemoteSetupDone(m_d->gdbServerPort, m_d->qmlPort);
}

void AbstractRemoteLinuxDebugSupport::handleRemoteProcessStarted()
{
    Q_ASSERT(m_d->debuggingType == RemoteLinuxRunConfiguration::DebugQmlOnly);
    QTC_ASSERT(m_d->state == StartingRemoteProcess, return);

    handleAdapterSetupDone();
}

void AbstractRemoteLinuxDebugSupport::setFinished()
{
    if (m_d->state == Inactive)
        return;
    m_d->state = Inactive;
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
      m_d(new RemoteLinuxDebugSupportPrivate(runConfig))
{
}

RemoteLinuxDebugSupport::~RemoteLinuxDebugSupport()
{
    delete m_d;
}

AbstractRemoteLinuxApplicationRunner *RemoteLinuxDebugSupport::runner() const
{
    return &m_d->runner;
}

} // namespace RemoteLinux
