/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "termgdbadapter.h"

#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtGui/QMessageBox>

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

TermGdbAdapter::TermGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
    m_stubProc.setMode(Utils::ConsoleProcess::Debug);
#ifdef Q_OS_UNIX
    m_stubProc.setSettings(Core::ICore::instance()->settings());
#endif

    connect(&m_stubProc, SIGNAL(processMessage(QString, bool)), SLOT(stubMessage(QString, bool)));
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

    m_stubProc.setWorkingDirectory(startParameters().workingDirectory);
    // Set environment + dumper preload.
    QStringList environment = startParameters().environment;
    m_stubProc.setEnvironment(environment);
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
    m_engine->notifyInferiorPid(attachedPID);
    m_engine->postCommand("attach " + QByteArray::number(attachedPID),
        CB(handleStubAttached));
}

void TermGdbAdapter::handleStubAttached(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        showMessage(_("INFERIOR ATTACHED"));
        m_engine->handleInferiorPrepared();
#ifdef Q_OS_LINUX
        m_engine->postCommand("-stack-list-frames 0 0", CB(handleEntryPoint));
#endif
    } else if (response.resultClass == GdbResultError) {
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        m_engine->notifyInferiorSetupFailed(msg);
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

void TermGdbAdapter::stubMessage(const QString &msg, bool)
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
