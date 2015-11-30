/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "termgdbadapter.h"

#include <debugger/debuggercore.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/shared/hostutils.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

using namespace Utils;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// TermGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbTermEngine::GdbTermEngine(const DebuggerRunParameters &startParameters)
    : GdbEngine(startParameters)
{
#ifdef Q_OS_WIN
    // Windows up to xp needs a workaround for attaching to freshly started processes. see proc_stub_win
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
        m_stubProc.setMode(ConsoleProcess::Suspend);
    else
        m_stubProc.setMode(ConsoleProcess::Debug);
#else
    m_stubProc.setMode(ConsoleProcess::Debug);
    m_stubProc.setSettings(Core::ICore::settings());
#endif
}

GdbTermEngine::~GdbTermEngine()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
}

void GdbTermEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

// Currently, adapters are not re-used
//    // We leave the console open, so recycle it now.
//    m_stubProc.blockSignals(true);
//    m_stubProc.stop();
//    m_stubProc.blockSignals(false);

    if (!prepareCommand())
        return;

    m_stubProc.setWorkingDirectory(runParameters().workingDirectory);
    // Set environment + dumper preload.
    m_stubProc.setEnvironment(runParameters().environment);

    connect(&m_stubProc, &ConsoleProcess::processError,
            this, &GdbTermEngine::stubError);
    connect(&m_stubProc, &ConsoleProcess::processStarted,
            this, &GdbTermEngine::stubStarted);
    connect(&m_stubProc, &ConsoleProcess::stubStopped,
            this, &GdbTermEngine::stubExited);
    // FIXME: Starting the stub implies starting the inferior. This is
    // fairly unclean as far as the state machine and error reporting go.

    if (!m_stubProc.start(runParameters().executable,
                         runParameters().processArgs)) {
        // Error message for user is delivered via a signal.
        handleAdapterStartFailed(QString());
        return;
    }
}

void GdbTermEngine::stubStarted()
{
    startGdb();
}

void GdbTermEngine::handleGdbStartFailed()
{
    m_stubProc.stop();
}

void GdbTermEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const qint64 attachedPID = m_stubProc.applicationPID();
    const qint64 attachedMainThreadID = m_stubProc.applicationMainThreadID();
    notifyInferiorPid(attachedPID);
    const QString msg = (attachedMainThreadID != -1)
            ? QString::fromLatin1("Going to attach to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID)
            : QString::fromLatin1("Going to attach to %1").arg(attachedPID);
    showMessage(msg, LogMisc);
    handleInferiorPrepared();
}

void GdbTermEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    const qint64 attachedPID = m_stubProc.applicationPID();
    runCommand({"attach " + QByteArray::number(attachedPID), NoFlags,
                [this](const DebuggerResponse &r) { handleStubAttached(r); }});
}

void GdbTermEngine::handleStubAttached(const DebuggerResponse &response)
{
    // InferiorStopOk can happen if the "*stopped" in response to the
    // 'attach' comes in before its '^done'
    QTC_ASSERT(state() == EngineRunRequested || state() == InferiorStopOk,
               qDebug() << state());

    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning:
        if (runParameters().toolChainAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            QString errorMessage;
            // Resume thread that was suspended by console stub process (see stub code).
            const qint64 mainThreadId = m_stubProc.applicationMainThreadID();
            if (winResumeThread(mainThreadId, &errorMessage)) {
                showMessage(QString::fromLatin1("Inferior attached, thread %1 resumed").
                            arg(mainThreadId), LogMisc);
            } else {
                showMessage(QString::fromLatin1("Inferior attached, unable to resume thread %1: %2").
                            arg(mainThreadId).arg(errorMessage),
                            LogWarning);
            }
            notifyEngineRunAndInferiorStopOk();
            continueInferiorInternal();
        } else {
            showMessage(_("INFERIOR ATTACHED AND RUNNING"));
            //notifyEngineRunAndInferiorRunOk();
            // Wait for the upcoming *stopped and handle it there.
        }
        break;
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            showMessage(msgPtraceError(runParameters().startMode));
            notifyEngineRunFailed();
            break;
        }
        showMessage(QString::fromLocal8Bit(response.data["msg"].data()));
        notifyEngineIll();
        break;
    default:
        showMessage(QString::fromLatin1("Invalid response %1").arg(response.resultClass));
        notifyEngineIll();
        break;
    }
}

void GdbTermEngine::interruptInferior2()
{
    interruptLocalInferior(inferiorPid());
}

void GdbTermEngine::stubError(const QString &msg)
{
    Core::AsynchronousMessageBox::critical(tr("Debugger Error"), msg);
}

void GdbTermEngine::stubExited()
{
    if (state() == EngineShutdownRequested || state() == DebuggerFinished) {
        showMessage(_("STUB EXITED EXPECTEDLY"));
        return;
    }
    showMessage(_("STUB EXITED"));
    notifyEngineIll();
}

void GdbTermEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
