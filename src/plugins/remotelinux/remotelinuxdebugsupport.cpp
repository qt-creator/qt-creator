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
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <utils/qtcassert.h>

#include <QPointer>

using namespace QSsh;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class LinuxDeviceDebugSupportPrivate
{
public:
    LinuxDeviceDebugSupportPrivate(const RemoteLinuxRunConfiguration *runConfig,
            DebuggerEngine *engine)
        : engine(engine),
          qmlDebugging(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger()),
          cppDebugging(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger()),
          gdbServerPort(-1), qmlPort(-1)
    {
    }

    const QPointer<DebuggerEngine> engine;
    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    int gdbServerPort;
    int qmlPort;
};

} // namespace Internal

using namespace Internal;

DebuggerStartParameters LinuxDeviceDebugSupport::startParameters(const RemoteLinuxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();
    const IDevice::ConstPtr device = DeviceKitInformation::device(k);
    QTC_ASSERT(device, return params);

    params.sysRoot = SysRootKitInformation::sysRoot(k).toString();
    params.debuggerCommand = DebuggerKitInformation::debuggerCommand(k).toString();
    if (ToolChain *tc = ToolChainKitInformation::toolChain(k))
        params.toolChainAbi = tc->targetAbi();

    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        params.languages |= QmlLanguage;
        params.qmlServerAddress = device->sshParameters().host;
        params.qmlServerPort = 0; // port is selected later on
    }
    if (aspect->useCppDebugger()) {
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

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RemoteLinuxRunConfiguration *runConfig,
        DebuggerEngine *engine)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
      d(new LinuxDeviceDebugSupportPrivate(static_cast<RemoteLinuxRunConfiguration *>(runConfig), engine))
{
    connect(d->engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleRemoteSetupRequested()));
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
    delete d;
}

void LinuxDeviceDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Inactive && d->engine)
        d->engine->showMessage(msg, channel);
}

void LinuxDeviceDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...\n"), LogStatus);
    AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested();
}

void LinuxDeviceDebugSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringPorts, return);

    if (d->cppDebugging && !setPort(d->gdbServerPort))
        return;
    if (d->qmlDebugging && !setPort(d->qmlPort))
            return;

    setState(StartingRunner);
    d->gdbserverOutput.clear();

    DeviceApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    if (d->qmlDebugging && !d->cppDebugging)
        connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    QString args = arguments();
    if (d->qmlDebugging)
        args += QString::fromLocal8Bit(" -qmljsdebugger=port:%1,block").arg(d->qmlPort);
    const QString remoteCommandLine = (d->qmlDebugging && !d->cppDebugging)
        ? QString::fromLatin1("%1 %2 %3").arg(commandPrefix()).arg(remoteFilePath()).arg(args)
        : QString::fromLatin1("%1 gdbserver :%2 %3 %4").arg(commandPrefix())
              .arg(d->gdbServerPort).arg(remoteFilePath()).arg(args);
    connect(runner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));
    runner->start(device(), remoteCommandLine.toUtf8());
}

void LinuxDeviceDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, AppError);
        if (d->engine)
            d->engine->notifyInferiorIll();
    } else if (state() != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void LinuxDeviceDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!d->engine || state() == Inactive)
        return;

    if (state() == Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            d->engine->quitDebugger();
        else if (!success)
            d->engine->notifyInferiorIll();

    } else if (state() == StartingRunner){
        d->engine->notifyEngineRemoteSetupFailed(tr("Debugging failed."));
    }
    reset();
}

void LinuxDeviceDebugSupport::handleDebuggingFinished()
{
    setFinished();
    reset();
}

void LinuxDeviceDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), AppOutput);
}

void LinuxDeviceDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);

    if (!d->engine)
        return;

    showMessage(QString::fromUtf8(output), AppError);
    if (state() == StartingRunner && d->cppDebugging) {
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
    AbstractRemoteLinuxRunSupport::handleAdapterSetupFailed(error);
    d->engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void LinuxDeviceDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupDone();
    d->engine->notifyEngineRemoteSetupDone(d->gdbServerPort, d->qmlPort);
}

void LinuxDeviceDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace RemoteLinux
