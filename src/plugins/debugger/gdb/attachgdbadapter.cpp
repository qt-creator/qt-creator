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

#include "attachgdbadapter.h"

#include "debuggeractions.h"
#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&AttachGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// AttachGdbAdapter
//
///////////////////////////////////////////////////////////////////////

AttachGdbAdapter::AttachGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
    commonInit();
}

void AttachGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    QStringList gdbArgs;
    gdbArgs.prepend(_("mi"));
    gdbArgs.prepend(_("-i"));

    QString location = theDebuggerStringSetting(GdbLocation);
    m_gdbProc.start(location, gdbArgs);
}

void AttachGdbAdapter::handleGdbStarted()
{
    QTC_ASSERT(state() == AdapterStarting, qDebug() << state());
    setState(AdapterStarted);
    emit adapterStarted();
}

void AttachGdbAdapter::handleGdbError(QProcess::ProcessError error)
{
    debugMessage(_("PLAIN ADAPTER, HANDLE GDB ERROR"));
    emit adapterCrashed(m_engine->errorMessage(error));
    shutdown();
}

void AttachGdbAdapter::prepareInferior()
{
    QTC_ASSERT(state() == AdapterStarted, qDebug() << state());
    setState(InferiorPreparing);
    setState(InferiorPrepared);
    emit inferiorPrepared();
}

void AttachGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    const qint64 pid = startParameters().attachPID;
    m_engine->postCommand(_("attach %1").arg(pid), CB(handleAttach));
    // Task 254674 does not want to remove them
    //qq->breakHandler()->removeAllBreakpoints();
}

void AttachGdbAdapter::handleAttach(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        setState(InferiorStopped);
        debugMessage(_("INFERIOR STARTED"));
        showStatusMessage(msgAttachedToStoppedInferior());
        m_engine->updateAll();
    } else if (response.resultClass == GdbResultError) {
        QString msg = __(response.data.findChild("msg").data());
        setState(InferiorStartFailed);
        emit inferiorStartFailed(msg);
    }
}

void AttachGdbAdapter::interruptInferior()
{
    debugMessage(_("TRYING TO INTERUPT INFERIOR"));
    const qint64 pid = startParameters().attachPID;
    if (!interruptProcess(pid))
        debugMessage(_("CANNOT INTERRUPT %1").arg(pid));
}

void AttachGdbAdapter::shutdown()
{
    switch (state()) {
    
    case InferiorStartFailed:
        m_engine->postCommand(_("-gdb-exit"));
        setState(DebuggerNotReady);
        return;

    case InferiorStopped:
        setState(InferiorShuttingDown);
        m_engine->postCommand(_("detach"), CB(handleDetach));
        return;

    case InferiorShutDown:
        setState(AdapterShuttingDown);
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;

    default:
        QTC_ASSERT(false, qDebug() << state());
    }
}

void AttachGdbAdapter::handleDetach(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        setState(InferiorShutDown);
        emit inferiorShutDown();
        shutdown(); // re-iterate...
    } else if (response.resultClass == GdbResultError) {
        const QString msg = msgInferiorStopFailed(__(response.data.findChild("msg").data()));
        setState(InferiorShutdownFailed);
        emit inferiorShutdownFailed(msg);
    }
}

void AttachGdbAdapter::handleExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // don't set state here, this will be handled in handleGdbFinished()
    } else if (response.resultClass == GdbResultError) {
        const QString msg = msgGdbStopFailed(__(response.data.findChild("msg").data()));
        emit adapterShutdownFailed(msg);
    }
}

void AttachGdbAdapter::handleGdbFinished(int, QProcess::ExitStatus)
{
    debugMessage(_("GDB PROESS FINISHED"));
    setState(DebuggerNotReady);
    emit adapterShutDown();
}

} // namespace Internal
} // namespace Debugger
