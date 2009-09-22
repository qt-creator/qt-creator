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

#include "remotegdbadapter.h"

#include "debuggeractions.h"
#include "gdbengine.h"
#include "procinterrupt.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&RemoteGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

RemoteGdbAdapter::RemoteGdbAdapter(GdbEngine *engine, QObject *parent)
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

void RemoteGdbAdapter::startAdapter(const DebuggerStartParametersPtr &sp)
{
    QTC_ASSERT(state() == AdapterNotRunning, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));
    m_startParameters = sp;

    QStringList gdbArgs;
    gdbArgs.prepend(_("mi"));
    gdbArgs.prepend(_("-i"));

    if (!m_engine->m_outputCollector.listen()) {
        emit adapterStartFailed(tr("Cannot set up communication with child process: %1")
                .arg(m_engine->m_outputCollector.errorString()));
        return;
    }
    gdbArgs.prepend(_("--tty=") + m_engine->m_outputCollector.serverName());

    if (!m_startParameters->workingDir.isEmpty())
        setWorkingDirectory(m_startParameters->workingDir);
    if (!m_startParameters->environment.isEmpty())
        setEnvironment(m_startParameters->environment);

    QString location = theDebuggerStringSetting(GdbLocation);
    //showStatusMessage(tr("Starting Debugger: ") + loc + _c(' ') + gdbArgs.join(_(" ")));
    m_gdbProc.start(location, gdbArgs);
}

void RemoteGdbAdapter::handleGdbStarted()
{
    QTC_ASSERT(state() == AdapterStarting, qDebug() << state());
    setState(AdapterStarted);
    emit adapterStarted();
}

void RemoteGdbAdapter::prepareInferior()
{
    QTC_ASSERT(state() == AdapterStarted, qDebug() << state());
    setState(InferiorPreparing);
    if (!m_startParameters->processArgs.isEmpty())
        m_engine->postCommand(_("-exec-arguments ")
            + m_startParameters->processArgs.join(_(" ")));
    QFileInfo fi(m_engine->startParameters().executable);
    m_engine->postCommand(_("-file-exec-and-symbols \"%1\"").arg(fi.absoluteFilePath()),
        CB(handleFileExecAndSymbols));
}

void RemoteGdbAdapter::handleFileExecAndSymbols(const GdbResultRecord &response, const QVariant &)
{
    QTC_ASSERT(state() == InferiorPreparing, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        //m_breakHandler->clearBreakMarkers();
        setState(InferiorPrepared);
        emit inferiorPrepared();
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Starting executable failed:\n") +
            __(response.data.findChild("msg").data());
        setState(InferiorPreparationFailed);
        emit inferiorPreparationFailed(msg);
    }
}

void RemoteGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorPrepared, qDebug() << state());
    setState(InferiorStarting);
    m_engine->postCommand(_("-exec-run"), CB(handleExecRun));
}

void RemoteGdbAdapter::handleExecRun(const GdbResultRecord &response, const QVariant &)
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        setState(InferiorStarted);
        emit inferiorStarted();
    } else {
        QTC_ASSERT(response.resultClass == GdbResultError, /**/);
        const QByteArray &msg = response.data.findChild("msg").data();
        //QTC_ASSERT(status() == DebuggerInferiorRunning, /**/);
        //interruptInferior();
        setState(InferiorStartFailed);
        emit inferiorStartFailed(msg);
    }
}

void RemoteGdbAdapter::interruptInferior()
{
    debugMessage(_("TRYING TO INTERUPT INFERIOR"));
    if (m_engine->startMode() == StartRemote) {
        m_engine->postCommand(_("-exec-interrupt"));
        return;
    }

    const qint64 attachedPID = m_engine->inferiorPid();
    if (attachedPID <= 0) {
        debugMessage(_("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"));
        return;
    }

    if (!interruptProcess(attachedPID))
        debugMessage(_("CANNOT INTERRUPT %1").arg(attachedPID));
}

void RemoteGdbAdapter::shutdownAdapter()
{
    if (state() == InferiorStarted) {
        setState(InferiorShuttingDown);
        m_engine->postCommand(_("kill"), CB(handleKill));
        return;
    }

    if (state() == InferiorShutDown) {
        setState(AdapterShuttingDown);
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;
    }

/*
    if (state() == InferiorShutdownFailed) {
        m_gdbProc.terminate();
        // 20s can easily happen when loading webkit debug information
        m_gdbProc.waitForFinished(20000);
        setState(AdapterShuttingDown);
        debugMessage(_("FORCING TERMINATION: %1")
            .arg(state()));
        if (state() != QProcess::NotRunning) {
            debugMessage(_("PROBLEM STOPPING DEBUGGER: STATE %1")
                .arg(state()));
            m_gdbProc.kill();
        }
        m_engine->postCommand(_("-gdb-exit"), CB(handleExit));
        return;
    }

*/
    QTC_ASSERT(state() == AdapterNotRunning, qDebug() << state());
}

void RemoteGdbAdapter::handleKill(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        setState(InferiorShutDown);
        emit inferiorShutDown();
        shutdownAdapter(); // re-iterate...
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Inferior process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        setState(InferiorShutdownFailed);
        emit inferiorShutdownFailed(msg);
    }
}

void RemoteGdbAdapter::handleExit(const GdbResultRecord &response, const QVariant &)
{
    if (response.resultClass == GdbResultDone) {
        // don't set state here, this will be handled in handleGdbFinished()
    } else if (response.resultClass == GdbResultError) {
        QString msg = tr("Gdb process could not be stopped:\n") +
            __(response.data.findChild("msg").data());
        emit adapterShutdownFailed(msg);
    }
}

void RemoteGdbAdapter::handleGdbFinished(int, QProcess::ExitStatus)
{
    debugMessage(_("GDB PROESS FINISHED"));
    setState(AdapterNotRunning);
    emit adapterShutDown();
}

void RemoteGdbAdapter::shutdownInferior()
{
    m_engine->postCommand(_("kill"));
}

} // namespace Internal
} // namespace Debugger
