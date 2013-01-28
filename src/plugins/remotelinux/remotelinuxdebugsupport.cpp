/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxdebugsupport.h"

#include "remotelinuxrunconfiguration.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QPointer>
#include <QSharedPointer>

using namespace QSsh;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, GatheringPorts, StartingRunner, Debugging };
} // anonymous namespace

class LinuxDeviceDebugSupportPrivate
{
public:
    LinuxDeviceDebugSupportPrivate(const RemoteLinuxRunConfiguration *runConfig,
            DebuggerEngine *engine)
        : engine(engine),
          qmlDebugging(runConfig->debuggerAspect()->useQmlDebugger()),
          cppDebugging(runConfig->debuggerAspect()->useCppDebugger()),
          state(Inactive),
          gdbServerPort(-1), qmlPort(-1),
          device(DeviceKitInformation::device(runConfig->target()->kit())),
          remoteFilePath(runConfig->remoteExecutableFilePath()),
          arguments(runConfig->arguments()),
          commandPrefix(runConfig->commandPrefix())
    {
    }

    const QPointer<DebuggerEngine> engine;
    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    State state;
    int gdbServerPort;
    int qmlPort;
    DeviceApplicationRunner appRunner;
    DeviceUsedPortsGatherer portsGatherer;
    const ProjectExplorer::IDevice::ConstPtr device;
    Utils::PortList portList;
    const QString remoteFilePath;
    const QString arguments;
    const QString commandPrefix;
};

} // namespace Internal

using namespace Internal;

DebuggerStartParameters LinuxDeviceDebugSupport::startParameters(const RemoteLinuxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();
    const IDevice::ConstPtr device = DeviceKitInformation::device(k);

    params.sysRoot = SysRootKitInformation::sysRoot(k).toString();
    params.debuggerCommand = DebuggerKitInformation::debuggerCommand(k).toString();
    if (ToolChain *tc = ToolChainKitInformation::toolChain(k))
        params.toolChainAbi = tc->targetAbi();

    if (runConfig->debuggerAspect()->useQmlDebugger()) {
        params.languages |= QmlLanguage;
        params.qmlServerAddress = device->sshParameters().host;
        params.qmlServerPort = 0; // port is selected later on
    }
    if (runConfig->debuggerAspect()->useCppDebugger()) {
        params.languages |= CppLanguage;
        params.processArgs = runConfig->arguments();
        params.startMode = AttachToRemoteServer;
        params.executable = runConfig->localExecutableFilePath();
        params.remoteChannel = device->sshParameters().host + QLatin1String(":-1");
    } else {
        params.startMode = AttachToRemoteServer;
    }
    params.remoteSetupNeeded = true;
    params.displayName = runConfig->displayName();

    if (const Project *project = target->project()) {
        params.projectSourceDirectory = project->projectDirectory();
        if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            params.projectBuildDirectory = buildConfig->buildDirectory();
        params.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
    }

    return params;
}

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunConfiguration *runConfig,
        DebuggerEngine *engine)
    : QObject(engine),
      d(new LinuxDeviceDebugSupportPrivate(static_cast<RemoteLinuxRunConfiguration *>(runConfig), engine))
{
    connect(d->engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleRemoteSetupRequested()));
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
    setFinished();
    delete d;
}

void LinuxDeviceDebugSupport::setApplicationRunnerPreRunAction(DeviceApplicationHelperAction *action)
{
    d->appRunner.setPreRunAction(action);
}

void LinuxDeviceDebugSupport::setApplicationRunnerPostRunAction(DeviceApplicationHelperAction *action)
{
    d->appRunner.setPostRunAction(action);
}

void LinuxDeviceDebugSupport::showMessage(const QString &msg, int channel)
{
    if (d->state != Inactive && d->engine)
        d->engine->showMessage(msg, channel);
}

void LinuxDeviceDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(d->state == Inactive, return);

    d->state = GatheringPorts;
    showMessage(tr("Checking available ports...\n"), LogStatus);
    connect(&d->portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(&d->portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));
    d->portsGatherer.start(d->device);
}

void LinuxDeviceDebugSupport::handlePortsGathererError(const QString &message)
{
    QTC_ASSERT(d->state == GatheringPorts, return);
    handleAdapterSetupFailed(message);
}

void LinuxDeviceDebugSupport::handlePortListReady()
{
    QTC_ASSERT(d->state == GatheringPorts, return);

    d->portList = d->device->freePorts();
    startExecution();
}

void LinuxDeviceDebugSupport::startExecution()
{
    QTC_ASSERT(d->state == GatheringPorts, return);

    if (d->cppDebugging && !setPort(d->gdbServerPort))
        return;
    if (d->qmlDebugging && !setPort(d->qmlPort))
            return;

    d->state = StartingRunner;
    d->gdbserverOutput.clear();

    connect(&d->appRunner, SIGNAL(remoteStderr(QByteArray)),
            SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(&d->appRunner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    if (d->qmlDebugging && !d->cppDebugging)
        connect(&d->appRunner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    QString args = d->arguments;
    if (d->qmlDebugging)
        args += QString::fromLocal8Bit(" -qmljsdebugger=port:%1,block").arg(d->qmlPort);
    const QString remoteCommandLine = (d->qmlDebugging && !d->cppDebugging)
        ? QString::fromLatin1("%1 %2 %3").arg(d->commandPrefix).arg(d->remoteFilePath).arg(args)
        : QString::fromLatin1("%1 gdbserver :%2 %3 %4").arg(d->commandPrefix)
              .arg(d->gdbServerPort).arg(d->remoteFilePath).arg(args);
    connect(&d->appRunner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    d->appRunner.start(d->device, remoteCommandLine.toUtf8());
}

void LinuxDeviceDebugSupport::handleAppRunnerError(const QString &error)
{
    if (d->state == Debugging) {
        showMessage(error, AppError);
        if (d->engine)
            d->engine->notifyInferiorIll();
    } else if (d->state != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void LinuxDeviceDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!d->engine || d->state == Inactive)
        return;

    if (d->state == Debugging) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            d->engine->quitDebugger();
        else if (!success)
            d->engine->notifyInferiorIll();

    } else {
        d->engine->notifyEngineRemoteSetupFailed(tr("Debugging failed."));
    }
}

void LinuxDeviceDebugSupport::handleDebuggingFinished()
{
    setFinished();
}

void LinuxDeviceDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(d->state == Inactive || d->state == Debugging, return);

    showMessage(QString::fromUtf8(output), AppOutput);
}

void LinuxDeviceDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(d->state != GatheringPorts, return);

    if (!d->engine)
        return;

    showMessage(QString::fromUtf8(output), AppError);
    if (d->state == StartingRunner && d->cppDebugging) {
        d->gdbserverOutput += output;
        if (d->gdbserverOutput.contains("Listening on port")) {
            handleAdapterSetupDone();
            d->gdbserverOutput.clear();
        }
    }
}

void LinuxDeviceDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), LogStatus);
}

void LinuxDeviceDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    setFinished();
    d->engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void LinuxDeviceDebugSupport::handleAdapterSetupDone()
{
    d->state = Debugging;
    d->engine->notifyEngineRemoteSetupDone(d->gdbServerPort, d->qmlPort);
}

void LinuxDeviceDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    QTC_ASSERT(d->state == StartingRunner, return);

    handleAdapterSetupDone();
}

void LinuxDeviceDebugSupport::setFinished()
{
    if (d->state == Inactive)
        return;
    d->portsGatherer.disconnect(this);
    d->appRunner.disconnect(this);
    if (d->state == StartingRunner) {
        const QString stopCommand
                = d->device->processSupport()->killProcessByNameCommandLine(d->remoteFilePath);
        d->appRunner.stop(stopCommand.toUtf8());
    }
    d->state = Inactive;
}

bool LinuxDeviceDebugSupport::setPort(int &port)
{
    port = d->portsGatherer.getNextFreePort(&d->portList);
    if (port == -1) {
        handleAdapterSetupFailed(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}

} // namespace RemoteLinux
