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

#include "remotelinuxanalyzesupport.h"

#include "remotelinuxrunconfiguration.h"

#include <analyzerbase/ianalyzerengine.h>

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
    RemoteLinuxAnalyzeSupportPrivate(IAnalyzerEngine *engine, RunMode runMode)
        : engine(engine),
          qmlProfiling(runMode == QmlProfilerRunMode),
          qmlPort(-1)
    {
    }

    const QPointer<IAnalyzerEngine> engine;
    bool qmlProfiling;
    int qmlPort;

    QmlDebug::QmlOutputParser outputParser;
};

} // namespace Internal

using namespace Internal;

AnalyzerStartParameters RemoteLinuxAnalyzeSupport::startParameters(const RemoteLinuxRunConfiguration *runConfig,
                                                                   RunMode runMode)
{
    AnalyzerStartParameters params;
    if (runMode == QmlProfilerRunMode)
        params.startMode = StartQmlRemote;
    params.connParams = DeviceKitInformation::device(runConfig->target()->kit())->sshParameters();
    params.analyzerCmdPrefix = runConfig->commandPrefix();
    params.displayName = runConfig->displayName();
    params.sysroot = SysRootKitInformation::sysRoot(runConfig->target()->kit()).toString();
    params.analyzerHost = params.connParams.host;

    return params;
}

RemoteLinuxAnalyzeSupport::RemoteLinuxAnalyzeSupport(RemoteLinuxRunConfiguration *runConfig,
                                                     IAnalyzerEngine *engine, RunMode runMode)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
      d(new RemoteLinuxAnalyzeSupportPrivate(engine, runMode))
{
    connect(d->engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)),
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
    if (state() != Inactive && d->engine)
        d->engine->logApplicationMessage(msg, format);
    d->outputParser.processOutput(msg);
}

void RemoteLinuxAnalyzeSupport::handleRemoteSetupRequested()
{
    if (d->engine->mode() != Analyzer::StartQmlRemote)
        return;

    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...\n"), Utils::NormalMessageFormat);
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

    const QString args = arguments()
            + QString::fromLocal8Bit(" -qmljsdebugger=port:%1,block").arg(d->qmlPort);
    const QString remoteCommandLine =
            QString::fromLatin1("%1 %2 %3").arg(commandPrefix()).arg(remoteFilePath()).arg(args);
    runner->start(device(), remoteCommandLine.toUtf8());
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
    d->engine->notifyRemoteFinished(success);
}

void RemoteLinuxAnalyzeSupport::handleProfilingFinished()
{
    if (d->engine->mode() != Analyzer::StartQmlRemote)
        return;
    setFinished();
}

void RemoteLinuxAnalyzeSupport::remoteIsRunning()
{
    d->engine->notifyRemoteSetupDone(d->qmlPort);
}

void RemoteLinuxAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), Utils::StdOutFormat);
}

void RemoteLinuxAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);

    if (!d->engine)
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
