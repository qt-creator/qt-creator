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

#include "androidanalyzesupport.h"

#include "androidrunner.h"
#include "androidmanager.h"

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>

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
                                                           RunMode runMode)
{
    Target *target = runConfig->target();
    AnalyzerStartParameters params;
    params.runMode = runMode;
    params.displayName = AndroidManager::packageName(target);
    params.sysroot = SysRootKitInformation::sysRoot(target->kit()).toString();
    // TODO: Not sure if these are the right paths.
    params.workingDirectory = target->project()->projectDirectory().toString();
    if (runMode == ProjectExplorer::QmlProfilerRunMode) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        params.analyzerHost = server.serverAddress().toString();
        params.startMode = StartLocal;
    }

    AnalyzerRunControl *analyzerRunControl = AnalyzerManager::createRunControl(params, runConfig);
    (void) new AndroidAnalyzeSupport(runConfig, analyzerRunControl);
    return analyzerRunControl;
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
        [this, runControl](const QByteArray &output) {
            const QString msg = QString::fromUtf8(output);
            runControl->logApplicationMessage(msg, Utils::StdErrFormatSameLine);
            m_outputParser.processOutput(msg);
        });

    connect(runner, &AndroidRunner::remoteOutput,
        [this, runControl](const QByteArray &output) {
            const QString msg = QString::fromUtf8(output);
            runControl->logApplicationMessage(msg, Utils::StdOutFormatSameLine);
            m_outputParser.processOutput(msg);
        });
}

} // namespace Internal
} // namespace Android
