/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "termgdbadapter.h"

#include "debuggerstartparameters.h"
#include "gdbmi.h"
#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"
#include "debuggercore.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtGui/QMessageBox>

#ifdef Q_OS_WIN
#    include "dbgwinutils.h"
#    include "dbgwinutils.h"
#endif

namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&TermGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// TermGdbAdapter
//
///////////////////////////////////////////////////////////////////////

TermGdbAdapter::TermGdbAdapter(GdbEngine *engine)
    : AbstractGdbAdapter(engine)
{
    m_stubProc.setMode(Utils::ConsoleProcess::Debug);
#ifdef Q_OS_UNIX
    m_stubProc.setSettings(Core::ICore::instance()->settings());
#endif

    connect(&m_stubProc, SIGNAL(processError(QString)), SLOT(stubError(QString)));
    connect(&m_stubProc, SIGNAL(processStarted()), SLOT(handleInferiorSetupOk()));
    connect(&m_stubProc, SIGNAL(wrapperStopped()), SLOT(stubExited()));
}

TermGdbAdapter::~TermGdbAdapter()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
}

AbstractGdbAdapter::DumperHandling TermGdbAdapter::dumperHandling() const
{
    // LD_PRELOAD fails for System-Qt on Mac.
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    return DumperLoadedByGdb;
#else
    return DumperLoadedByAdapter; // Handles loading itself via LD_PRELOAD
#endif
}

void TermGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

// Currently, adapters are not re-used
//    // We leave the console open, so recycle it now.
//    m_stubProc.blockSignals(true);
//    m_stubProc.stop();
//    m_stubProc.blockSignals(false);

#ifdef Q_OS_WIN
    if (!prepareCommand())
        return;
#endif

    m_stubProc.setWorkingDirectory(startParameters().workingDirectory);
    // Set environment + dumper preload.
    m_stubProc.setEnvironment(startParameters().environment);
    // FIXME: Starting the stub implies starting the inferior. This is
    // fairly unclean as far as the state machine and error reporting go.
    if (!m_stubProc.start(startParameters().executable,
                         startParameters().processArgs)) {
        // Error message for user is delivered via a signal.
        m_engine->handleAdapterStartFailed(QString(), QString());
        return;
    }

    if (!m_engine->startGdb()) {
        m_stubProc.stop();
        return;
    }
}

void TermGdbAdapter::handleInferiorSetupOk()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    m_engine->handleAdapterStarted();
}

void TermGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const qint64 attachedPID = m_stubProc.applicationPID();
#ifdef Q_OS_WIN
    const qint64 attachedMainThreadID = m_stubProc.applicationMainThreadID();
    showMessage(QString::fromLatin1("Attaching to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID), LogMisc);
#else
    showMessage(QString::fromLatin1("Attaching to %1").arg(attachedPID), LogMisc);
#endif
    m_engine->notifyInferiorPid(attachedPID);
    m_engine->postCommand("attach " + QByteArray::number(attachedPID),
        CB(handleStubAttached));
}

void TermGdbAdapter::handleStubAttached(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
#ifdef Q_OS_WIN
    QString errorMessage;
#endif // Q_OS_WIN
    switch (response.resultClass) {
    case GdbResultDone:
    case GdbResultRunning:
#ifdef Q_OS_WIN
        // Resume thread that was suspended by console stub process (see stub code).
        if (winResumeThread(m_stubProc.applicationMainThreadID(), &errorMessage)) {
            showMessage(QString::fromLatin1("Inferior attached, thread %1 resumed").
                        arg(m_stubProc.applicationMainThreadID()), LogMisc);
        } else {
            showMessage(QString::fromLatin1("Inferior attached, unable to resume thread %1: %2").
                        arg(m_stubProc.applicationMainThreadID()).arg(errorMessage),
                        LogWarning);
        }
#else
        showMessage(_("INFERIOR ATTACHED"));
#endif // Q_OS_WIN
        m_engine->handleInferiorPrepared();
#ifdef Q_OS_LINUX
        m_engine->postCommand("-stack-list-frames 0 0", CB(handleEntryPoint));
#endif
        break;
    case GdbResultError:
        m_engine->notifyInferiorSetupFailed(QString::fromLocal8Bit(response.data.findChild("msg").data()));
        break;
    default:
        m_engine->notifyInferiorSetupFailed(QString::fromLatin1("Invalid response %1").arg(response.resultClass));
        break;
    }
}

void TermGdbAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyEngineRunAndInferiorStopOk();
    m_engine->continueInferiorInternal();
}

#ifdef Q_OS_LINUX
void TermGdbAdapter::handleEntryPoint(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        GdbMi stack = response.data.findChild("stack");
        if (stack.isValid() && stack.childCount() == 1)
            m_engine->m_entryPoint = stack.childAt(0).findChild("addr").data();
    }
}
#endif

void TermGdbAdapter::interruptInferior()
{
    const qint64 attachedPID = m_engine->inferiorPid();
    QTC_ASSERT(attachedPID > 0, return);
    if (!interruptProcess(attachedPID))
        showMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void TermGdbAdapter::stubError(const QString &msg)
{
    showMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
}

void TermGdbAdapter::stubExited()
{
    if (state() == EngineShutdownRequested || state() == DebuggerFinished) {
        showMessage(_("STUB EXITED EXPECTEDLY"));
        return;
    }
    showMessage(_("STUB EXITED"));
    m_engine->notifyEngineIll();
}

void TermGdbAdapter::shutdownInferior()
{
    m_engine->defaultInferiorShutdown("kill");
}

void TermGdbAdapter::shutdownAdapter()
{
    m_engine->notifyAdapterShutdownOk();
}

} // namespace Internal
} // namespace Debugger
