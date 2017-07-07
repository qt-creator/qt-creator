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

#include "qnxanalyzesupport.h"

#include "qnxdevice.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qmldebug/qmloutputparser.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

class QnxAnalyzeeRunner : public SimpleTargetRunner
{
public:
    QnxAnalyzeeRunner(RunControl *runControl, PortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
    {
        setDisplayName("QnxAnalyzeeRunner");
    }

private:
    void start() override
    {
        Utils::Port port = m_portsGatherer->findPort();

        auto r = runnable().as<StandardRunnable>();
        if (!r.commandLineArguments.isEmpty())
            r.commandLineArguments += ' ';
        r.commandLineArguments +=
                QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices, port);

        setRunnable(r);

        SimpleTargetRunner::start();
    }

    PortsGatherer *m_portsGatherer;
};


// QnxDebugSupport

QnxQmlProfilerSupport::QnxQmlProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    runControl->createWorker(runControl->runMode());

    setDisplayName("QnxAnalyzeSupport");
    appendMessage(tr("Preparing remote side..."), Utils::LogMessageFormat);

    auto portsGatherer = new PortsGatherer(runControl);

    auto debuggeeRunner = new QnxAnalyzeeRunner(runControl, portsGatherer);
    debuggeeRunner->addDependency(portsGatherer);

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    slog2InfoRunner->addDependency(debuggeeRunner);

    addDependency(slog2InfoRunner);

    // QmlDebug::QmlOutputParser m_outputParser;
    // FIXME: m_outputParser needs to be fed with application output
    //    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
    //            this, &QnxAnalyzeSupport::remoteIsRunning);

    //    m_outputParser.processOutput(msg);
}

void QnxQmlProfilerSupport::start()
{
    // runControl()->notifyRemoteSetupDone(m_qmlPort);
    reportStarted();
}

} // namespace Internal
} // namespace Qnx
