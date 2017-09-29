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

#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

QnxQmlProfilerSupport::QnxQmlProfilerSupport(RunControl *runControl)
    : SimpleTargetRunner(runControl)
{
    setDisplayName("QnxQmlProfilerSupport");
    appendMessage(tr("Preparing remote side..."), Utils::LogMessageFormat);

    m_portsGatherer = new PortsGatherer(runControl);
    addStartDependency(m_portsGatherer);

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    addStartDependency(slog2InfoRunner);

    m_profiler = runControl->createWorker(runControl->runMode());
    m_profiler->addStartDependency(this);
    addStopDependency(m_profiler);
}

void QnxQmlProfilerSupport::start()
{
    Port qmlPort = m_portsGatherer->findPort();

    QUrl serverUrl;
    serverUrl.setHost(device()->sshParameters().host);
    serverUrl.setPort(qmlPort.number());
    serverUrl.setScheme("tcp");
    m_profiler->recordData("QmlServerUrl", serverUrl);

    QString args = QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices, qmlPort);
    auto r = runnable().as<StandardRunnable>();
    if (!r.commandLineArguments.isEmpty())
        r.commandLineArguments.append(' ');
    r.commandLineArguments += args;

    setRunnable(r);

    SimpleTargetRunner::start();
}

} // namespace Internal
} // namespace Qnx
