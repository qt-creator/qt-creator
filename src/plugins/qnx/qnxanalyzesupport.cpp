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

#include "qnxanalyzesupport.h"

#include <analyzerbase/ianalyzerengine.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

using namespace Qnx;
using namespace Qnx::Internal;

QnxAnalyzeSupport::QnxAnalyzeSupport(QnxRunConfiguration *runConfig,
                                     Analyzer::IAnalyzerEngine *engine)
    : QnxAbstractRunSupport(runConfig, engine)
    , m_engine(engine)
    , m_qmlPort(-1)
{
    const DeviceApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleError(QString)));
    connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    connect(runner, SIGNAL(finished(bool)), SLOT(handleRemoteProcessFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));

    connect(m_engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)),
            SLOT(handleAdapterSetupRequested()));
    connect(&m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            SLOT(remoteIsRunning()));
}

void QnxAnalyzeSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Preparing remote side...\n"), Utils::NormalMessageFormat);
    QnxAbstractRunSupport::handleAdapterSetupRequested();
}

void QnxAnalyzeSupport::startExecution()
{
    if (state() == Inactive)
        return;

    if (!setPort(m_qmlPort) && m_qmlPort == -1)
        return;

    setState(StartingRemoteProcess);

    const QString args = m_engine->startParameters().debuggeeArgs +
            QString::fromLatin1(" -qmljsdebugger=port:%1,block").arg(m_qmlPort);
    const QString command = QString::fromLatin1("%1 %2 %3").arg(commandPrefix(), executable(), args);
    appRunner()->start(device(), command.toUtf8());
}

void QnxAnalyzeSupport::handleRemoteProcessFinished(bool success)
{
    if (m_engine || state() == Inactive)
        return;

    if (!success)
        showMessage(tr("The %1 process closed unexpectedly.").arg(executable()),
                    Utils::NormalMessageFormat);
    m_engine->notifyRemoteFinished(success);
}

void QnxAnalyzeSupport::handleProfilingFinished()
{
    setFinished();
}

void QnxAnalyzeSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

void QnxAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), Utils::StdOutFormat);
}

void QnxAnalyzeSupport::handleError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, Utils::ErrorMessageFormat);
    } else if (state() != Inactive) {
        showMessage(tr("Initial setup failed: %1").arg(error), Utils::NormalMessageFormat);
        setFinished();
    }
}

void QnxAnalyzeSupport::remoteIsRunning()
{
    if (m_engine)
        m_engine->notifyRemoteSetupDone(m_qmlPort);
}

void QnxAnalyzeSupport::showMessage(const QString &msg, Utils::OutputFormat format)
{
    if (state() != Inactive && m_engine)
        m_engine->logApplicationMessage(msg, format);
    m_outputParser.processOutput(msg);
}
