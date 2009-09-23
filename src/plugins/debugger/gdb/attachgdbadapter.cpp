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

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
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
    QTC_ASSERT(state() == AdapterNotRunning, qDebug() << state());
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        this, SIGNAL(error(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        this, SIGNAL(readyReadStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        this, SIGNAL(readyReadStandardError()));
    connect(&m_gdbProc, SIGNAL(started()),
        this, SLOT(handleGdbStarted()));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
}

void AttachGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == AdapterNotRunning, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    QStringList gdbArgs;
    gdbArgs.prepend(_("mi"));
    gdbArgs.prepend(_("-i"));

    if (!m_engine->m_outputCollector.listen()) {
        emit adapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_engine->m_outputCollector.errorString()));
        return;
    }
    gdbArgs.prepend(_("--tty=") + m_engine->m_outputCollector.serverName());

    if (!startParameters().workingDir.isEmpty())
        setWorkingDirectory(startParameters().workingDir);
    if (!startParameters().environment.isEmpty())
        setEnvironment(startParameters().environment);

    QString location = theDebuggerStringSetting(GdbLocation);
    m_gdbProc.start(location, gdbArgs);
}

void AttachGdbAdapter::handleGdbStarted()
{
    QTC_ASSERT(state() == AdapterStarting, qDebug() << state());
    setState(AdapterStarted);
    emit adapterStarted();
}

void AttachGdbAdapter::prepareInferior()
{
    const qint64 pid = startParameters().attachPID;
    QTC_ASSERT(state() == AdapterStarted, qDebug() << state());
    setState(InferiorPreparing);
    qDebug() << "USING " << pid;
    m_engine->postCommand(_("attach %1").arg(pid), CB(handleAttach));
    // Task 254674 does not want to remove them
    //qq->breakHandler()->removeAllBreakpoints();
}

void AttachGdbAdapter::handleAttach(const GdbResultRecord &response, const QVariant &)
{
    QTC_ASSERT(state() == InferiorPreparing, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        setState(InferiorPrepared);
        emit inferiorPrepared();
    } else if (response.resultClass == GdbResultError) {
        QString msg = __(response.data.findChild("msg").data());
        setState(InferiorPreparationFailed);
        emit inferiorPreparationFailed(msg);
    }
}

void AttachGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorPrepared, qDebug() << state());
    setState(InferiorStarting);
    m_engine->postCommand(_("-exec-continue"), CB(handleContinue));
}

void AttachGdbAdapter::handleContinue(const GdbResultRecord &response, const QVariant &)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        setState(InferiorStarted);
        emit inferiorStarted();
    } else {
        QTC_ASSERT(response.resultClass == GdbResultError, /**/);
        const QByteArray &msg = response.data.findChild("msg").data();
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
    if (state() == InferiorStarted) {
        setState(InferiorShuttingDown);
        m_engine->postCommand(_("detach"), CB(handleDetach));
        return;
    }

    if (state() == InferiorShutDown) {
        setState(AdapterShuttingDown);
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;
    }
    QTC_ASSERT(state() == AdapterNotRunning, qDebug() << state());
}

void AttachGdbAdapter::handleDetach(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        setState(InferiorShutDown);
        emit inferiorShutDown();
        shutdown(); // re-iterate...
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Inferior process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        setState(InferiorShutdownFailed);
        emit inferiorShutdownFailed(msg);
    }
}

void AttachGdbAdapter::handleExit(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        // don't set state here, this will be handled in handleGdbFinished()
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Gdb process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        emit adapterShutdownFailed(msg);
    }
}

void AttachGdbAdapter::handleGdbFinished(int, QProcess::ExitStatus)
{
    debugMessage(_("GDB PROESS FINISHED"));
    setState(AdapterNotRunning);
    emit adapterShutDown();
}

} // namespace Internal
} // namespace Debugger
