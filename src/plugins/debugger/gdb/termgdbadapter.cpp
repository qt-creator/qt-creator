/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

    connect(&m_stubProc, SIGNAL(processError(QString)), SLOT(stubError(QString)));
    connect(&m_stubProc, SIGNAL(processStarted()), SLOT(handleInferiorStarted()));
    connect(&m_stubProc, SIGNAL(wrapperStopped()), SLOT(stubExited()));
}

TermGdbAdapter::~TermGdbAdapter()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
}

void TermGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

// Currently, adapters are not re-used
//    // We leave the console open, so recycle it now.
//    m_stubProc.blockSignals(true);
//    m_stubProc.stop();
//    m_stubProc.blockSignals(false);

    m_stubProc.setWorkingDirectory(startParameters().workingDir);
    m_stubProc.setEnvironment(startParameters().environment);
    // FIXME: Starting the stub implies starting the inferior. This is
    // fairly unclean as far as the state machine and error reporting go.
    if (!m_stubProc.start(startParameters().executable,
                         startParameters().processArgs)) {
        // Error message for user is delivered via a signal.
        emit adapterStartFailed(QString(), QString());
        return;
    }

    if (!m_engine->startGdb()) {
        m_stubProc.stop();
        return;
    }
}

void TermGdbAdapter::handleInferiorStarted()
{
    QTC_ASSERT(state() == AdapterStarting, qDebug() << state());
    emit adapterStarted();
}

void TermGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    const qint64 attachedPID = m_stubProc.applicationPID();
    m_engine->handleInferiorPidChanged(attachedPID);
    m_engine->postCommand(_("attach %1").arg(attachedPID), CB(handleStubAttached));
}

void TermGdbAdapter::handleStubAttached(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        setState(InferiorStopped);
        debugMessage(_("INFERIOR ATTACHED"));
        emit inferiorPrepared();
#ifdef Q_OS_LINUX
        m_engine->postCommand(_("-stack-list-frames 0 0"), CB(handleEntryPoint));
#endif
    } else if (response.resultClass == GdbResultError) {
        QString msg = _(response.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

void TermGdbAdapter::startInferiorPhase2()
{
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
        debugMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void TermGdbAdapter::stubError(const QString &msg)
{
    showMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
}

void TermGdbAdapter::stubExited()
{
    debugMessage(_("STUB EXITED"));
    if (state() != AdapterStarting // From previous instance
        && state() != EngineShuttingDown && state() != DebuggerNotReady)
        emit adapterCrashed(QString());
}

} // namespace Internal
} // namespace Debugger
