/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxdebugsupport.h"

#include "remotelinuxrunconfiguration.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QPointer>

using namespace QSsh;
using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class LinuxDeviceDebugSupportPrivate
{
public:
    LinuxDeviceDebugSupportPrivate(const AbstractRemoteLinuxRunConfiguration *runConfig,
            DebuggerRunControl *runControl)
        : runControl(runControl),
          qmlDebugging(runConfig->extraAspect<DebuggerRunConfigurationAspect>()->useQmlDebugger()),
          cppDebugging(runConfig->extraAspect<DebuggerRunConfigurationAspect>()->useCppDebugger()),
          gdbServerPort(-1), qmlPort(-1)
    {
    }

    const QPointer<DebuggerRunControl> runControl;
    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    int gdbServerPort;
    int qmlPort;
};

} // namespace Internal

using namespace Internal;

DebuggerStartParameters LinuxDeviceDebugSupport::startParameters(const AbstractRemoteLinuxRunConfiguration *runConfig)
{
    DebuggerStartParameters params;
    Target *target = runConfig->target();
    Kit *k = target->kit();
    const IDevice::ConstPtr device = DeviceKitInformation::device(k);
    QTC_ASSERT(device, return params);

    params.startMode = AttachToRemoteServer;
    params.closeMode = KillAndExitMonitorAtClose;
    params.remoteSetupNeeded = true;
    params.displayName = runConfig->displayName();

    auto aspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        params.qmlServerAddress = device->sshParameters().host;
        params.qmlServerPort = 0; // port is selected later on
    }
    if (aspect->useCppDebugger()) {
        aspect->setUseMultiProcess(true);
        QStringList args = runConfig->arguments();
        if (aspect->useQmlDebugger())
            args.prepend(QString::fromLatin1("-qmljsdebugger=port:%qml_port%,block"));
        params.processArgs = Utils::QtcProcess::joinArgs(args, Utils::OsTypeLinux);
        params.executable = runConfig->localExecutableFilePath();
        params.remoteChannel = device->sshParameters().host + QLatin1String(":-1");
        params.remoteExecutable = runConfig->remoteExecutableFilePath();
    }

    return params;
}

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(AbstractRemoteLinuxRunConfiguration *runConfig,
        DebuggerRunControl *runControl)
    : AbstractRemoteLinuxRunSupport(runConfig, runControl),
      d(new LinuxDeviceDebugSupportPrivate(static_cast<AbstractRemoteLinuxRunConfiguration *>(runConfig), runControl))
{
    connect(runControl, &DebuggerRunControl::requestRemoteSetup,
            this, &LinuxDeviceDebugSupport::handleRemoteSetupRequested);
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
    delete d;
}

void LinuxDeviceDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Inactive && d->runControl)
        d->runControl->showMessage(msg, channel);
}

void LinuxDeviceDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), LogStatus);
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
    connect(runner, &DeviceApplicationRunner::remoteStderr,
            this, &LinuxDeviceDebugSupport::handleRemoteErrorOutput);
    connect(runner, &DeviceApplicationRunner::remoteStdout,
            this, &LinuxDeviceDebugSupport::handleRemoteOutput);
    if (d->qmlDebugging && !d->cppDebugging)
        connect(runner, &DeviceApplicationRunner::remoteProcessStarted,
                this, &LinuxDeviceDebugSupport::handleRemoteProcessStarted);

    QStringList args = arguments();
    QString command;

    if (d->qmlDebugging)
        args.prepend(QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(d->qmlPort));

    if (d->qmlDebugging && !d->cppDebugging) {
        command = remoteFilePath();
    } else {
        command = device()->debugServerPath();
        if (command.isEmpty())
            command = QLatin1String("gdbserver");
        args.clear();
        args.append(QString::fromLatin1("--multi"));
        args.append(QString::fromLatin1(":%1").arg(d->gdbServerPort));
    }

    connect(runner, &DeviceApplicationRunner::finished,
            this, &LinuxDeviceDebugSupport::handleAppRunnerFinished);
    connect(runner, &DeviceApplicationRunner::reportProgress,
            this, &LinuxDeviceDebugSupport::handleProgressReport);
    connect(runner, &DeviceApplicationRunner::reportError,
            this, &LinuxDeviceDebugSupport::handleAppRunnerError);
    runner->setEnvironment(environment());
    runner->setWorkingDirectory(workingDirectory());
    runner->start(device(), command, args);
}

void LinuxDeviceDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, AppError);
        if (d->runControl)
            d->runControl->notifyInferiorIll();
    } else if (state() != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void LinuxDeviceDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!d->runControl || state() == Inactive)
        return;

    if (state() == Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            d->runControl->quitDebugger();
        else if (!success)
            d->runControl->notifyInferiorIll();

    } else if (state() == StartingRunner) {
        RemoteSetupResult result;
        result.success = false;
        result.reason = tr("Debugging failed.");
        d->runControl->notifyEngineRemoteSetupFinished(result);
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

    if (!d->runControl)
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

    RemoteSetupResult result;
    result.success = false;
    result.reason = tr("Initial setup failed: %1").arg(error);
    d->runControl->notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupDone();

    RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = d->gdbServerPort;
    result.qmlServerPort = d->qmlPort;
    d->runControl->notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace RemoteLinux
