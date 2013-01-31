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

#include "termgdbadapter.h"

#include "debuggercore.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "procinterrupt.h"
#include "shared/hostutils.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QMessageBox>

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::GdbCommandCallback>(&GdbTermEngine::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// TermGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbTermEngine::GdbTermEngine(const DebuggerStartParameters &startParameters)
    : GdbEngine(startParameters)
{
#ifdef Q_OS_WIN
    // Windows up to xp needs a workaround for attaching to freshly started processes. see proc_stub_win
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
        m_stubProc.setMode(Utils::ConsoleProcess::Suspend);
    else
        m_stubProc.setMode(Utils::ConsoleProcess::Debug);
#else
    m_stubProc.setMode(Utils::ConsoleProcess::Debug);
    m_stubProc.setSettings(Core::ICore::settings());
#endif
}

GdbTermEngine::~GdbTermEngine()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
}

GdbEngine::DumperHandling GdbTermEngine::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
    return Utils::HostOsInfo::isWindowsHost() || Utils::HostOsInfo::isMacHost()
            ? DumperLoadedByGdb
            : DumperLoadedByAdapter; // Handles loading itself via LD_PRELOAD
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

    m_stubProc.setWorkingDirectory(startParameters().workingDirectory);
    // Set environment + dumper preload.
    m_stubProc.setEnvironment(startParameters().environment);

    connect(&m_stubProc, SIGNAL(processError(QString)), SLOT(stubError(QString)));
    connect(&m_stubProc, SIGNAL(processStarted()), SLOT(stubStarted()));
    connect(&m_stubProc, SIGNAL(stubStopped()), SLOT(stubExited()));
    // FIXME: Starting the stub implies starting the inferior. This is
    // fairly unclean as far as the state machine and error reporting go.

    if (!m_stubProc.start(startParameters().executable,
                         startParameters().processArgs)) {
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
#ifdef Q_OS_WIN
    const qint64 attachedMainThreadID = m_stubProc.applicationMainThreadID();
    showMessage(QString::fromLatin1("Attaching to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID), LogMisc);
#else
    showMessage(QString::fromLatin1("Attaching to %1").arg(attachedPID), LogMisc);
#endif
    notifyInferiorPid(attachedPID);
    postCommand("attach " + QByteArray::number(attachedPID),
        CB(handleStubAttached));
}

void GdbTermEngine::handleStubAttached(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    switch (response.resultClass) {
    case GdbResultDone:
    case GdbResultRunning:
        if (startParameters().toolChainAbi.os() != ProjectExplorer::Abi::WindowsOS) {
            showMessage(_("INFERIOR ATTACHED"));
        } else {
            QString errorMessage;
            // Resume thread that was suspended by console stub process (see stub code).
#ifdef Q_OS_WIN
            const qint64 mainThreadId = m_stubProc.applicationMainThreadID();
#else
            const qint64 mainThreadId = -1;
#endif
            if (winResumeThread(mainThreadId, &errorMessage)) {
                showMessage(QString::fromLatin1("Inferior attached, thread %1 resumed").
                            arg(mainThreadId), LogMisc);
            } else {
                showMessage(QString::fromLatin1("Inferior attached, unable to resume thread %1: %2").
                            arg(mainThreadId).arg(errorMessage),
                            LogWarning);
            }
        }
        handleInferiorPrepared();
        break;
    case GdbResultError:
        if (response.data.findChild("msg").data() == "ptrace: Operation not permitted.") {
            notifyInferiorSetupFailed(DumperHelper::msgPtraceError(startParameters().startMode));
            break;
        }
        notifyInferiorSetupFailed(QString::fromLocal8Bit(response.data.findChild("msg").data()));
        break;
    default:
        notifyInferiorSetupFailed(QString::fromLatin1("Invalid response %1").arg(response.resultClass));
        break;
    }
}

void GdbTermEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    notifyEngineRunAndInferiorStopOk();
    continueInferiorInternal();
}

void GdbTermEngine::interruptInferior2()
{
    interruptLocalInferior(inferiorPid());
}

void GdbTermEngine::stubError(const QString &msg)
{
    showMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
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
