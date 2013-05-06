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

#include "androidanalyzesupport.h"

#include "androidrunner.h"
#include "androidmanager.h"

#include "analyzerbase/ianalyzertool.h"
#include "analyzerbase/ianalyzerengine.h"
#include "analyzerbase/analyzermanager.h"
#include "analyzerbase/analyzerruncontrol.h"
#include "analyzerbase/analyzerstartparameters.h"

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
                                                           RunMode runMode, QString *errorMessage)
{
    IAnalyzerTool *tool = AnalyzerManager::toolFromRunMode(runMode);
    if (!tool) {
        if (errorMessage)
            *errorMessage = tr("No analyzer tool selected");
        return 0;
    }

    AnalyzerStartParameters params;
    params.toolId = tool->id();
    params.startMode = StartQmlRemote;
    Target *target = runConfig->target();
    params.displayName = AndroidManager::packageName(target);
    params.sysroot = SysRootKitInformation::sysRoot(target->kit()).toString();
    // TODO: Not sure if these are the right paths.
    params.workingDirectory = target->project()->projectDirectory();
    if (runMode == ProjectExplorer::QmlProfilerRunMode) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return 0);
        params.analyzerHost = server.serverAddress().toString();
    }

    AnalyzerRunControl * const analyzerRunControl = new AnalyzerRunControl(tool, params, runConfig);
    new AndroidAnalyzeSupport(runConfig, analyzerRunControl);
    return analyzerRunControl;
}

AndroidAnalyzeSupport::AndroidAnalyzeSupport(AndroidRunConfiguration *runConfig,
    AnalyzerRunControl *runControl)
    : AndroidRunSupport(runConfig, runControl),
      m_engine(0)
{
    if (runControl) {
        m_engine = runControl->engine();
        if (m_engine) {
            connect(m_engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)),
                    m_runner, SLOT(start()));
        }
    }
    connect(m_runner, SIGNAL(remoteProcessStarted(int)),
            SLOT(handleRemoteProcessStarted(int)));
}

void AndroidAnalyzeSupport::handleRemoteProcessStarted(int qmlPort)
{
    if (m_engine)
        m_engine->notifyRemoteSetupDone(qmlPort);
}

void AndroidAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    if (m_engine)
        m_engine->logApplicationMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
    else
        AndroidRunSupport::handleRemoteOutput(output);
}

void AndroidAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    if (m_engine)
        m_engine->logApplicationMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
    else
        AndroidRunSupport::handleRemoteErrorOutput(output);
}

} // namespace Internal
} // namespace Android
