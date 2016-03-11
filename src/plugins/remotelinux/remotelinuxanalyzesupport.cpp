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

#include "remotelinuxanalyzesupport.h"

#include "remotelinuxrunconfiguration.h"

#include <debugger/analyzer/analyzerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qmldebug/qmloutputparser.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QPointer>

using namespace QSsh;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxAnalyzeSupportPrivate
{
public:
    RemoteLinuxAnalyzeSupportPrivate(AnalyzerRunControl *rc, Core::Id runMode)
        : runControl(rc),
          qmlProfiling(runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE),
          qmlPort(-1)
    {
    }

    const QPointer<AnalyzerRunControl> runControl;
    bool qmlProfiling;
    int qmlPort;

    QmlDebug::QmlOutputParser outputParser;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxAnalyzeSupport::RemoteLinuxAnalyzeSupport(RunConfiguration *runConfig,
                                                     AnalyzerRunControl *engine, Core::Id runMode)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
      d(new RemoteLinuxAnalyzeSupportPrivate(engine, runMode))
{
    connect(d->runControl.data(), &AnalyzerRunControl::starting,
            this, &RemoteLinuxAnalyzeSupport::handleRemoteSetupRequested);
    connect(&d->outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &RemoteLinuxAnalyzeSupport::remoteIsRunning);
    connect(engine, &RunControl::finished,
            this, &RemoteLinuxAnalyzeSupport::handleProfilingFinished);
}

RemoteLinuxAnalyzeSupport::~RemoteLinuxAnalyzeSupport()
{
    delete d;
}

void RemoteLinuxAnalyzeSupport::showMessage(const QString &msg, Utils::OutputFormat format)
{
    if (state() != Inactive && d->runControl)
        d->runControl->logApplicationMessage(msg, format);
    d->outputParser.processOutput(msg);
}

void RemoteLinuxAnalyzeSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Utils::NormalMessageFormat);
    AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested();
}

void RemoteLinuxAnalyzeSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringPorts, return);

    // Currently we support only QML profiling
    QTC_ASSERT(d->qmlProfiling, return);

    if (!setPort(d->qmlPort))
        return;

    setState(StartingRunner);

    DeviceApplicationRunner *runner = appRunner();
    connect(runner, &DeviceApplicationRunner::remoteStderr,
            this, &RemoteLinuxAnalyzeSupport::handleRemoteErrorOutput);
    connect(runner, &DeviceApplicationRunner::remoteStdout,
            this, &RemoteLinuxAnalyzeSupport::handleRemoteOutput);
    connect(runner, &DeviceApplicationRunner::remoteProcessStarted,
            this, &RemoteLinuxAnalyzeSupport::handleRemoteProcessStarted);
    connect(runner, &DeviceApplicationRunner::finished,
            this, &RemoteLinuxAnalyzeSupport::handleAppRunnerFinished);
    connect(runner, &DeviceApplicationRunner::reportProgress,
            this, &RemoteLinuxAnalyzeSupport::handleProgressReport);
    connect(runner, &DeviceApplicationRunner::reportError,
            this, &RemoteLinuxAnalyzeSupport::handleAppRunnerError);

    auto r = runnable();
    if (!r.commandLineArguments.isEmpty())
        r.commandLineArguments.append(QLatin1Char(' '));
    r.commandLineArguments
            += QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices, d->qmlPort);
    runner->start(device(), r);
}

void RemoteLinuxAnalyzeSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running)
        showMessage(error, Utils::ErrorMessageFormat);
    else if (state() != Inactive)
        handleAdapterSetupFailed(error);
}

void RemoteLinuxAnalyzeSupport::handleAppRunnerFinished(bool success)
{
    // reset needs to be called first to ensure that the correct state is set.
    reset();
    if (!success)
        showMessage(tr("Failure running remote process."), Utils::NormalMessageFormat);
    d->runControl->notifyRemoteFinished();
}

void RemoteLinuxAnalyzeSupport::handleProfilingFinished()
{
    setFinished();
}

void RemoteLinuxAnalyzeSupport::remoteIsRunning()
{
    d->runControl->notifyRemoteSetupDone(d->qmlPort);
}

void RemoteLinuxAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), Utils::StdOutFormat);
}

void RemoteLinuxAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);

    if (!d->runControl)
        return;

    showMessage(QString::fromUtf8(output), Utils::StdErrFormat);
}

void RemoteLinuxAnalyzeSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

void RemoteLinuxAnalyzeSupport::handleAdapterSetupFailed(const QString &error)
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupFailed(error);
    showMessage(tr("Initial setup failed: %1").arg(error), Utils::NormalMessageFormat);
}

void RemoteLinuxAnalyzeSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlProfiling, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace RemoteLinux
