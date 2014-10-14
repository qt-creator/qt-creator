/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxanalyzesupport.h"

#include "remotelinuxrunconfiguration.h"

#include <analyzerbase/analyzerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>

#include <utils/qtcassert.h>
#include <qmldebug/qmloutputparser.h>

#include <QPointer>

using namespace QSsh;
using namespace Analyzer;
using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxAnalyzeSupportPrivate
{
public:
    RemoteLinuxAnalyzeSupportPrivate(AnalyzerRunControl *rc, RunMode runMode)
        : runControl(rc),
          qmlProfiling(runMode == QmlProfilerRunMode),
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

AnalyzerStartParameters RemoteLinuxAnalyzeSupport::startParameters(const AbstractRemoteLinuxRunConfiguration *runConfig,
                                                                   RunMode runMode)
{
    AnalyzerStartParameters params;
    if (runMode == QmlProfilerRunMode)
        params.startMode = StartLocal;
    params.runMode = runMode;
    params.connParams = DeviceKitInformation::device(runConfig->target()->kit())->sshParameters();
    params.displayName = runConfig->displayName();
    params.sysroot = SysRootKitInformation::sysRoot(runConfig->target()->kit()).toString();
    params.analyzerHost = params.connParams.host;

    return params;
}

RemoteLinuxAnalyzeSupport::RemoteLinuxAnalyzeSupport(AbstractRemoteLinuxRunConfiguration *runConfig,
                                                     AnalyzerRunControl *engine, RunMode runMode)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
      d(new RemoteLinuxAnalyzeSupportPrivate(engine, runMode))
{
    connect(d->runControl, SIGNAL(starting(const Analyzer::AnalyzerRunControl*)),
            SLOT(handleRemoteSetupRequested()));
    connect(&d->outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            SLOT(remoteIsRunning()));
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
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    connect(runner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));

    const QStringList args = arguments()
            << QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(d->qmlPort);
    runner->setWorkingDirectory(workingDirectory());
    runner->setEnvironment(environment());
    runner->start(device(), remoteFilePath(), args);
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
