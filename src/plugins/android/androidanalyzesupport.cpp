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
    : AndroidRunSupport(runConfig, runControl),
      m_runControl(0),
      m_qmlPort(0)
{
    if (runControl) {
        m_runControl = runControl;
        connect(m_runControl, SIGNAL(starting(const Analyzer::AnalyzerRunControl*)),
                m_runner, SLOT(start()));
    }
    connect(&m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            SLOT(remoteIsRunning()));
    connect(m_runner, SIGNAL(remoteProcessStarted(int)),
            SLOT(handleRemoteProcessStarted(int)));
    connect(m_runner, SIGNAL(remoteProcessFinished(QString)),
            SLOT(handleRemoteProcessFinished(QString)));

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
            SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
            SLOT(handleRemoteOutput(QByteArray)));
}

void AndroidAnalyzeSupport::handleRemoteProcessStarted(int qmlPort)
{
    m_qmlPort = qmlPort;
}

void AndroidAnalyzeSupport::handleRemoteProcessFinished(const QString &errorMsg)
{
    if (m_runControl)
        m_runControl->notifyRemoteFinished();
    AndroidRunSupport::handleRemoteProcessFinished(errorMsg);
}

void AndroidAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    const QString msg = QString::fromUtf8(output);
    if (m_runControl)
        m_runControl->logApplicationMessage(msg, Utils::StdOutFormatSameLine);
    else
        AndroidRunSupport::handleRemoteOutput(output);
    m_outputParser.processOutput(msg);
}

void AndroidAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    const QString msg = QString::fromUtf8(output);
    if (m_runControl)
        m_runControl->logApplicationMessage(msg, Utils::StdErrFormatSameLine);
    else
        AndroidRunSupport::handleRemoteErrorOutput(output);
    m_outputParser.processOutput(msg);
}

void AndroidAnalyzeSupport::remoteIsRunning()
{
    if (m_runControl)
        m_runControl->notifyRemoteSetupDone(m_qmlPort);
}

} // namespace Internal
} // namespace Android
