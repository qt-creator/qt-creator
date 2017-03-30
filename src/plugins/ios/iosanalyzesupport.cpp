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

#include "iosanalyzesupport.h"
#include "iosrunner.h"

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosAnalyzeSupport::IosAnalyzeSupport(RunControl *runControl, bool cppDebug, bool qmlDebug)
    : ToolRunner(runControl),
      m_runner(new IosRunner(this, runControl, cppDebug, qmlDebug ? QmlDebug::QmlProfilerServices :
                                                                   QmlDebug::NoQmlDebugServices))
{
    connect(runControl, &RunControl::starting,
            m_runner, &IosRunner::start);
    connect(runControl, &RunControl::finished,
            m_runner, &IosRunner::stop);
    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &IosAnalyzeSupport::qmlServerReady);

    connect(m_runner, &IosRunner::gotServerPorts,
        this, &IosAnalyzeSupport::handleServerPorts);
    connect(m_runner, &IosRunner::gotInferiorPid,
        this, &IosAnalyzeSupport::handleGotInferiorPid);
    connect(m_runner, &IosRunner::finished,
        this, &IosAnalyzeSupport::handleRemoteProcessFinished);

    connect(m_runner, &IosRunner::errorMsg,
        this, &IosAnalyzeSupport::handleRemoteErrorOutput);
    connect(m_runner, &IosRunner::appOutput,
        this, &IosAnalyzeSupport::handleRemoteOutput);
}

void IosAnalyzeSupport::qmlServerReady()
{
    runControl()->notifyRemoteSetupDone(m_qmlPort);
}

void IosAnalyzeSupport::handleServerPorts(Utils::Port gdbServerPort, Utils::Port qmlPort)
{
    Q_UNUSED(gdbServerPort);
    m_qmlPort = qmlPort;
}

void IosAnalyzeSupport::handleGotInferiorPid(qint64 pid, Utils::Port qmlPort)
{
    Q_UNUSED(pid);
    m_qmlPort = qmlPort;
}

void IosAnalyzeSupport::handleRemoteProcessFinished(bool cleanEnd)
{
    if (!cleanEnd)
        appendMessage(tr("Run ended with error."), Utils::ErrorMessageFormat);
    else
        appendMessage(tr("Run ended."), Utils::NormalMessageFormat);
    runControl()->notifyRemoteFinished();
}

void IosAnalyzeSupport::handleRemoteOutput(const QString &output)
{
    appendMessage(output, Utils::StdOutFormat);
    m_outputParser.processOutput(output);
}

void IosAnalyzeSupport::handleRemoteErrorOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormat);
    m_outputParser.processOutput(output);
}

} // namespace Internal
} // namespace Ios
