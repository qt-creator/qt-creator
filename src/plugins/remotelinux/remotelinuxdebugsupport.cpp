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

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>

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
    QByteArray gdbserverOutput;
    Port gdbServerPort;
    Port qmlPort;
};

} // namespace Internal

using namespace Internal;

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunControl *runControl,
                                                 const Debugger::DebuggerStartParameters &sp,
                                                 QString *errorMessage)
    : DebuggerRunTool(runControl, sp, errorMessage),
      d(new LinuxDeviceDebugSupportPrivate)
{
    connect(this, &DebuggerRunTool::requestRemoteSetup,
            this, &LinuxDeviceDebugSupport::handleRemoteSetupRequested);
    connect(runControl, &RunControl::finished,
            this, &LinuxDeviceDebugSupport::handleDebuggingFinished);

    connect(qobject_cast<AbstractRemoteLinuxRunSupport *>(runControl->targetRunner()),
            &AbstractRemoteLinuxRunSupport::executionStartRequested,
            this, &LinuxDeviceDebugSupport::startExecution);
    connect(qobject_cast<AbstractRemoteLinuxRunSupport *>(runControl->targetRunner()),
            &AbstractRemoteLinuxRunSupport::adapterSetupFailed,
            this, &LinuxDeviceDebugSupport::handleAdapterSetupFailed);
}

LinuxDeviceDebugSupport::~LinuxDeviceDebugSupport()
{
    delete d;
}

AbstractRemoteLinuxRunSupport *LinuxDeviceDebugSupport::targetRunner() const
{
    return qobject_cast<AbstractRemoteLinuxRunSupport *>(runControl()->targetRunner());
}

AbstractRemoteLinuxRunSupport::State LinuxDeviceDebugSupport::state() const
{
    AbstractRemoteLinuxRunSupport *runner = targetRunner();
    return runner ? runner->state() : AbstractRemoteLinuxRunSupport::Inactive;
}

void LinuxDeviceDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == AbstractRemoteLinuxRunSupport::Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), LogStatus);
    targetRunner()->startPortsGathering();
}

void LinuxDeviceDebugSupport::startExecution()
{
    QTC_ASSERT(state() == AbstractRemoteLinuxRunSupport::GatheringResources, return);

    if (isCppDebugging()) {
        d->gdbServerPort = targetRunner()->findPort();
        if (!d->gdbServerPort.isValid()) {
            handleAdapterSetupFailed(tr("Not enough free ports on device for C++ debugging."));
            return;
        }
    }
    if (isQmlDebugging()) {
        d->qmlPort = targetRunner()->findPort();
        if (!d->qmlPort.isValid()) {
            handleAdapterSetupFailed(tr("Not enough free ports on device for QML debugging."));
            return;
        }
    }

    targetRunner()->setState(AbstractRemoteLinuxRunSupport::StartingRunner);
    d->gdbserverOutput.clear();

    ApplicationLauncher *launcher = targetRunner()->applicationLauncher();
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
    if (isQmlDebugging() && !isCppDebugging())
        connect(launcher, &ApplicationLauncher::remoteProcessStarted,
                this, &LinuxDeviceDebugSupport::handleRemoteProcessStarted);

    launcher->start(realRunnable(), device());
}

Runnable LinuxDeviceDebugSupport::realRunnable() const
{
    StandardRunnable r = runControl()->runnable().as<StandardRunnable>();
    QStringList args = QtcProcess::splitArgs(r.commandLineArguments, OsTypeLinux);
    QString command;

    if (isQmlDebugging())
        args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices, d->qmlPort));

    if (isQmlDebugging() && !isCppDebugging()) {
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
    if (state() == AbstractRemoteLinuxRunSupport::Running) {
        showMessage(error, AppError);
        notifyInferiorIll();
    } else if (state() != AbstractRemoteLinuxRunSupport::Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void LinuxDeviceDebugSupport::handleAppRunnerFinished(bool success)
{
    if (state() == AbstractRemoteLinuxRunSupport::Inactive)
        return;

    if (state() == AbstractRemoteLinuxRunSupport::Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (isQmlDebugging() && !isCppDebugging())
            quitDebugger();
        else if (!success)
            notifyInferiorIll();

    } else if (state() == AbstractRemoteLinuxRunSupport::StartingRunner) {
        RemoteSetupResult result;
        result.success = false;
        result.reason = tr("Debugging failed.");
        notifyEngineRemoteSetupFinished(result);
    }
    targetRunner()->reset();
}

void LinuxDeviceDebugSupport::handleDebuggingFinished()
{
    targetRunner()->setFinished();
    targetRunner()->reset();
}

void LinuxDeviceDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == AbstractRemoteLinuxRunSupport::Inactive
            || state() == AbstractRemoteLinuxRunSupport::Running, return);

    showMessage(QString::fromUtf8(output), AppOutput);
}

void LinuxDeviceDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != AbstractRemoteLinuxRunSupport::GatheringResources, return);

    showMessage(QString::fromUtf8(output), AppError);
    if (state() == AbstractRemoteLinuxRunSupport::StartingRunner && isCppDebugging()) {
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
    RemoteSetupResult result;
    result.success = false;
    result.reason = tr("Initial setup failed: %1").arg(error);
    notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleAdapterSetupDone()
{
    targetRunner()->setState(AbstractRemoteLinuxRunSupport::Running);

    RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = d->gdbServerPort;
    result.qmlServerPort = d->qmlPort;
    notifyEngineRemoteSetupFinished(result);
}

void LinuxDeviceDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(isQmlDebugging() && !isCppDebugging(), return);
    QTC_ASSERT(state() == AbstractRemoteLinuxRunSupport::StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace RemoteLinux
