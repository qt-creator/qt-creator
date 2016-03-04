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

#include "androidanalyzesupport.h"

#include "androidrunner.h"
#include "androidmanager.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <debugger/analyzer/analyzerstartparameters.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>
#include <QTcpServer>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {

RunControl *AndroidAnalyzeSupport::createAnalyzeRunControl(AndroidRunConfiguration *runConfig,
                                                           Core::Id runMode)
{
    AnalyzerRunControl *runControl = AnalyzerManager::createRunControl(runConfig, runMode);
    QTC_ASSERT(runControl, return 0);
    AnalyzerConnection connection;
    if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        connection.analyzerHost = server.serverAddress().toString();
    }
    runControl->setDisplayName(AndroidManager::packageName(runConfig->target()));
    runControl->setConnection(connection);
    (void) new AndroidAnalyzeSupport(runConfig, runControl);
    return runControl;
}

AndroidAnalyzeSupport::AndroidAnalyzeSupport(AndroidRunConfiguration *runConfig,
    AnalyzerRunControl *runControl)
    : QObject(runControl),
      m_qmlPort(0)
{
    QTC_ASSERT(runControl, return);

    auto runner = new AndroidRunner(this, runConfig, runControl->runMode());

    connect(runControl, &AnalyzerRunControl::finished,
        [runner]() { runner->stop(); });

    connect(runControl, &AnalyzerRunControl::starting,
        [runner]() { runner->start(); });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
        [this, runControl](quint16) {
            runControl->notifyRemoteSetupDone(m_qmlPort);
        });

    connect(runner, &AndroidRunner::remoteProcessStarted,
        [this](int, int qmlPort) {
            m_qmlPort = qmlPort;
        });

    connect(runner, &AndroidRunner::remoteProcessFinished,
        [this, runControl](const QString &errorMsg)  {
            runControl->notifyRemoteFinished();
            runControl->appendMessage(errorMsg, Utils::NormalMessageFormat);
        });

    connect(runner, &AndroidRunner::remoteErrorOutput,
        [this, runControl](const QString &msg) {
            runControl->logApplicationMessage(msg, Utils::StdErrFormatSameLine);
            m_outputParser.processOutput(msg);
        });

    connect(runner, &AndroidRunner::remoteOutput,
        [this, runControl](const QString &msg) {
            runControl->logApplicationMessage(msg, Utils::StdOutFormatSameLine);
            m_outputParser.processOutput(msg);
        });
}

} // namespace Internal
} // namespace Android
