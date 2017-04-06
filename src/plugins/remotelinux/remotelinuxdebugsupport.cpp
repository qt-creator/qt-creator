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

#include "remotelinuxdebugsupport.h"

#include "remotelinuxrunconfiguration.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QPointer>

using namespace QSsh;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class LinuxDeviceDebugSupportPrivate
{
public:
    LinuxDeviceDebugSupportPrivate(const RunConfiguration *runConfig)
        : qmlDebugging(runConfig->extraAspect<DebuggerRunConfigurationAspect>()->useQmlDebugger()),
          cppDebugging(runConfig->extraAspect<DebuggerRunConfigurationAspect>()->useCppDebugger())
    {
    }

    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    Port gdbServerPort;
    Port qmlPort;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunControl *runControl)
    : AbstractRemoteLinuxRunSupport(runControl),
      d(new LinuxDeviceDebugSupportPrivate(runControl->runConfiguration()))
{
    connect(this->runControl(), &DebuggerRunControl::requestRemoteSetup,
            this, &LinuxDeviceDebugSupport::handleRemoteSetupRequested);
    connect(runControl, &RunControl::finished,
            this, &LinuxDeviceDebugSupport::handleDebuggingFinished);
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
    delete d;
}

bool LinuxDeviceDebugSupport::isCppDebugging() const
{
    return d->cppDebugging;
}

bool LinuxDeviceDebugSupport::isQmlDebugging() const
{
    return d->qmlDebugging;
}

void LinuxDeviceDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Inactive)
        runControl()->showMessage(msg, channel);
}

DebuggerRunControl *LinuxDeviceDebugSupport::runControl() const
{
    return qobject_cast<DebuggerRunControl *>(AbstractRemoteLinuxRunSupport::runControl());
}

void LinuxDeviceDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), LogStatus);
    startPortsGathering();
}

void LinuxDeviceDebugSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringResources, return);

    if (d->cppDebugging) {
        d->gdbServerPort = findPort();
        if (!d->gdbServerPort.isValid()) {
            handleAdapterSetupFailed(tr("Not enough free ports on device for C++ debugging."));
            return;
        }
    }
    if (d->qmlDebugging) {
        d->qmlPort = findPort();
        if (!d->qmlPort.isValid()) {
            handleAdapterSetupFailed(tr("Not enough free ports on device for QML debugging."));
            return;
        }
    }

    setState(StartingRunner);
    d->gdbserverOutput.clear();

    ApplicationLauncher *launcher = appRunner();
    connect(launcher, &ApplicationLauncher::remoteStderr,
            this, &LinuxDeviceDebugSupport::handleRemoteErrorOutput);
    connect(launcher, &ApplicationLauncher::remoteStdout,
            this, &LinuxDeviceDebugSupport::handleRemoteOutput);
    connect(launcher, &ApplicationLauncher::finished,
            this, &LinuxDeviceDebugSupport::handleAppRunnerFinished);
    connect(launcher, &ApplicationLauncher::reportProgress,
            this, &LinuxDeviceDebugSupport::handleProgressReport);
    connect(launcher, &ApplicationLauncher::reportError,
            this, &LinuxDeviceDebugSupport::handleAppRunnerError);
    if (d->qmlDebugging && !d->cppDebugging)
        connect(launcher, &ApplicationLauncher::remoteProcessStarted,
                this, &LinuxDeviceDebugSupport::handleRemoteProcessStarted);

    launcher->start(realRunnable(), device());
}

Runnable LinuxDeviceDebugSupport::realRunnable() const
{
    StandardRunnable r = runControl()->runnable().as<StandardRunnable>();
    QStringList args = QtcProcess::splitArgs(r.commandLineArguments, OsTypeLinux);
    QString command;

    if (d->qmlDebugging)
        args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices, d->qmlPort));

    if (d->qmlDebugging && !d->cppDebugging) {
        command = r.executable;
    } else {
        command = device()->debugServerPath();
        if (command.isEmpty())
            command = QLatin1String("gdbserver");
        args.clear();
        args.append(QString::fromLatin1("--multi"));
        args.append(QString::fromLatin1(":%1").arg(d->gdbServerPort.number()));
    }
    r.executable = command;
    r.commandLineArguments = QtcProcess::joinArgs(args, OsTypeLinux);
    return r;
}

void LinuxDeviceDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, AppError);
        runControl()->notifyInferiorIll();
    } else if (state() != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void LinuxDeviceDebugSupport::handleAppRunnerFinished(bool success)
{
    if (state() == Inactive)
        return;

    if (state() == Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            runControl()->quitDebugger();
        else if (!success)
            runControl()->notifyInferiorIll();

    } else if (state() == StartingRunner) {
        RemoteSetupResult result;
        result.success = false;
        result.reason = tr("Debugging failed.");
        runControl()->notifyEngineRemoteSetupFinished(result);
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
    QTC_ASSERT(state() != GatheringResources, return);

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
    runControl()->notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupDone();

    RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = d->gdbServerPort;
    result.qmlServerPort = d->qmlPort;
    runControl()->notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace RemoteLinux
