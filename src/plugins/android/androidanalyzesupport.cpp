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

#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidAnalyzeSupport::AndroidAnalyzeSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("AndroidAnalyzeSupport");

    RunConfiguration *runConfig = runControl->runConfiguration();
    runControl->setDisplayName(AndroidManager::packageName(runConfig->target()));
    runControl->setConnection(UrlConnection::localHostWithoutPort());

    auto runner = new AndroidRunner(runControl);

    connect(runControl, &RunControl::finished, runner, [runner] { runner->stop(); });

    connect(runControl, &RunControl::starting, runner, [runner] { runner->start(); });

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort, this,
        [this, runControl](Utils::Port) {
            runControl->notifyRemoteSetupDone(m_qmlPort);
        });

//    connect(runner, &AndroidRunner::handleRemoteProcessStarted, this,
//        [this](Utils::Port, Utils::Port qmlPort) {
//            m_qmlPort = qmlPort;
//        });

//    connect(runner, &AndroidRunner::handleRemoteProcessFinished, this,
//        [this, runControl](const QString &errorMsg)  {
//            runControl->notifyRemoteFinished();
//            appendMessage(errorMsg, Utils::NormalMessageFormat);
//        });

    connect(runner, &AndroidRunner::remoteErrorOutput, this,
        [this, runControl](const QString &msg) {
            appendMessage(msg, Utils::StdErrFormatSameLine);
            m_outputParser.processOutput(msg);
        });

    connect(runner, &AndroidRunner::remoteOutput, this,
        [this, runControl](const QString &msg) {
            appendMessage(msg, Utils::StdOutFormatSameLine);
            m_outputParser.processOutput(msg);
        });
}

} // namespace Internal
} // namespace Android
