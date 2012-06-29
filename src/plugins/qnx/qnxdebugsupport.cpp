/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxdebugsupport.h"
#include "qnxapplicationrunner.h"
#include "qnxconstants.h"

#include <debugger/debuggerengine.h>
#include <remotelinux/remotelinuxusedportsgatherer.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDebugSupport::QnxDebugSupport(QnxRunConfiguration *runConfig, Debugger::DebuggerEngine *engine)
    : QObject(engine)
    , m_engine(engine)
    , m_port(-1)
    , m_state(Inactive)
{
    m_runner = new QnxApplicationRunner(runConfig, this);
    m_runner->setDebugMode(true);

    connect(m_engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleAdapterSetupRequested()));
}

void QnxDebugSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(m_state == Inactive, return);

    m_state = StartingRunner;
    if (m_engine)
        m_engine->showMessage(tr("Preparing remote side...\n"), Debugger::AppStuff);

    connect(m_runner, SIGNAL(error(QString)),                this, SLOT(handleSshError(QString)));
    connect(m_runner, SIGNAL(readyForExecution()),           this, SLOT(startExecution()));
    connect(m_runner, SIGNAL(remoteProcessStarted()),        this, SLOT(handleRemoteProcessStarted()));
    connect(m_runner, SIGNAL(remoteProcessFinished(qint64)), this, SLOT(handleRemoteProcessFinished(qint64)));
    connect(m_runner, SIGNAL(reportProgress(QString)),       this, SLOT(handleProgressReport(QString)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),      this, SLOT(handleRemoteOutput(QByteArray)));

    m_runner->start();
}

void QnxDebugSupport::startExecution()
{
    if (m_state == Inactive)
        return;

    QTC_ASSERT(m_state == StartingRunner, return);

    m_state = StartingRemoteProcess;
    m_port = m_runner->usedPortsGatherer()->getNextFreePort(m_runner->freePorts());

    const QString remoteCommandLine = QString::fromLatin1("%1 %2 %3").arg(m_runner->commandPrefix()).arg(QLatin1String(Constants::QNX_DEBUG_EXECUTABLE)).arg(m_port);
    m_runner->startExecution(remoteCommandLine.toUtf8());
}

void QnxDebugSupport::handleRemoteProcessStarted()
{
    if (m_engine)
        m_engine->notifyEngineRemoteSetupDone(m_port, -1);
}

void QnxDebugSupport::handleRemoteProcessFinished(qint64 exitCode)
{
    if (m_engine || m_state == Inactive)
        return;

    if (m_state == Debugging) {
        if (exitCode != 0)
            m_engine->notifyInferiorIll();

    } else {
        const QString errorMsg = tr("The %1 process closed unexpectedly.").arg(QLatin1String(Constants::QNX_DEBUG_EXECUTABLE));
        m_engine->notifyEngineRemoteSetupFailed(errorMsg);
    }
}

void QnxDebugSupport::handleDebuggingFinished()
{
    setFinished();
}

void QnxDebugSupport::setFinished()
{
    m_state = Inactive;
    m_runner->stop();
}

void QnxDebugSupport::handleProgressReport(const QString &progressOutput)
{
    if (m_engine)
        m_engine->showMessage(progressOutput + QLatin1Char('\n'), Debugger::AppStuff);
}

void QnxDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(m_state == Inactive || m_state == Debugging, return);

    if (m_engine)
        m_engine->showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void QnxDebugSupport::handleSshError(const QString &error)
{
    if (m_state == Debugging) {
        if (m_engine) {
            m_engine->showMessage(error, Debugger::AppError);
            m_engine->notifyInferiorIll();
        }
    } else if (m_state != Inactive) {
        setFinished();
        if (m_engine)
            m_engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
    }
}
