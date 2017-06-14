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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <ssh/sshconnection.h>

#include <qmldebug/qmloutputparser.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QPointer>

using namespace QSsh;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

// RemoteLinuxQmlProfilerSupport

RemoteLinuxQmlProfilerSupport::RemoteLinuxQmlProfilerSupport(RunControl *runControl)
    : SimpleTargetRunner(runControl)
{
    setDisplayName("RemoteLinuxQmlProfilerSupport");

    m_portsGatherer = new PortsGatherer(runControl);
    addDependency(m_portsGatherer);

    m_profiler = runControl->createWorker(runControl->runMode());
    m_profiler->addDependency(this);
}

void RemoteLinuxQmlProfilerSupport::start()
{
    Port qmlPort = m_portsGatherer->findPort();

    QUrl serverUrl;
    serverUrl.setHost(device()->sshParameters().host);
    serverUrl.setPort(qmlPort.number());
    m_profiler->recordData("QmlServerUrl", serverUrl);

    QString args = QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices, qmlPort);
    auto r = runnable().as<StandardRunnable>();
    if (!r.commandLineArguments.isEmpty())
        r.commandLineArguments.append(' ');
    r.commandLineArguments += args;

    setRunnable(r);

    SimpleTargetRunner::start();
}


// RemoteLinuxPerfSupport

RemoteLinuxPerfSupport::RemoteLinuxPerfSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("RemoteLinuxPerfSupport");

    RunConfiguration *runConfiguration = runControl->runConfiguration();
    QTC_ASSERT(runConfiguration, return);
    IRunConfigurationAspect *perfAspect =
            runConfiguration->extraAspect("Analyzer.Perf.Settings");
    QTC_ASSERT(perfAspect, return);
    m_perfRecordArguments =
            perfAspect->currentSettings()->property("perfRecordArguments").toStringList()
            .join(' ');

    auto toolRunner = runControl->createWorker(runControl->runMode());
    toolRunner->addDependency(this);
//    connect(&m_outputGatherer, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
//            this, &RemoteLinuxPerfSupport::remoteIsRunning);

//    addDependency(FifoCreatorWorkerId);
}

void RemoteLinuxPerfSupport::start()
{
//    m_remoteFifo = targetRunner()->fifo();
    if (m_remoteFifo.isEmpty()) {
        reportFailure(tr("FIFO for profiling data could not be created."));
        return;
    }

//    ApplicationLauncher *runner = targetRunner()->applicationLauncher();

    auto r = runnable().as<StandardRunnable>();

    r.commandLineArguments = "-c 'perf record -o - " + m_perfRecordArguments
            + " -- " + r.executable + " "
            + r.commandLineArguments + " > " + m_remoteFifo
            + "'";
    r.executable = "sh";

    connect(&m_outputGatherer, SIGNAL(remoteStdout(QByteArray)),
            runControl(), SIGNAL(analyzePerfOutput(QByteArray)));
    connect(&m_outputGatherer, SIGNAL(finished(bool)),
            runControl(), SIGNAL(perfFinished()));

    StandardRunnable outputRunner;
    outputRunner.executable = "sh";
    outputRunner.commandLineArguments = QString("-c 'cat %1 && rm -r `dirname %1`'").arg(m_remoteFifo);
    m_outputGatherer.start(outputRunner, device());
//    remoteIsRunning();
//    runControl()->notifyRemoteSetupDone(d->qmlPort);

//    runner->start(r, device());
}

} // namespace RemoteLinux
